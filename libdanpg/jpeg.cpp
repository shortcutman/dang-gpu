//
//  jpeg.cpp
//  danpg
//
//  Created by Daniel Burke on 2/5/2023.
//

#include "jpeg.hpp"

#include "colour.hpp"
#include "idct.hpp"

#include <cstddef>
#include <iostream>
#include <bit>
#include <map>
#include <fstream>
#include <numbers>
#include <cmath>
#include <strstream>
#include <cassert>
#include <cstring>

#include <arpa/inet.h>

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

using namespace image;

namespace {

template<typename T>
T getAs(void* data) {
    return *reinterpret_cast<T*>(data);
}

}

Jpeg::Jpeg(std::span<uint8_t> is, MTL::Device* metalDevice) : _metalDevice(metalDevice) {
    size_t position = 0;
    
    while (position < is.size()) {
        auto data = std::span{is.begin() + position, is.size() - position};
        if (!_inScan) {
            position += readData(data);
        } else {
            position += readScanData(data);
        }
    }    
}

Jpeg::Jpeg() {
    
}

size_t Jpeg::readData(std::span<uint8_t> is) {
    uint8_t step = is[0];
    if (step == 0xff) {
        uint8_t markerCode = is[1];
        
        if (markerCode == 0xd8) {
            std::cout << "\tSOI Start of Image, skipping byte count" << std::endl;
            return 2;
        }
        
        uint16_t segmentBytes;
        segmentBytes = *reinterpret_cast<uint16_t*>(&is[2]);
        segmentBytes = htons(segmentBytes);
        segmentBytes -= 2;
        
        std::cout << "\tSegment has " << segmentBytes << " bytes";
        
        std::span<uint8_t> data(&is[4], segmentBytes);
        
        switch (markerCode) {
            case 0xe0:
                appZeroData(data);
                break;
                
            case 0xdb:
                quantisationTable(data);
                break;
                
            case 0xc0:
                sofBaselineDCT(data);
                break;
                
            case 0xc4:
                huffmanTable(data);
                break;
                
            case 0xda:
                startOfScan(data);
                break;
                
            case 0xdd:
                restartInterval(data);
                break;
                
            default:
                std::cout << "\tUnhandled marker segment" << std::endl;
                break;
        }
        
        return 4 + segmentBytes;
    }
    
    return 1;
}

namespace {

int extend_op(int v, int t) {
    auto vt = 1 << (t - 1);
    if (v < vt) {
        vt = 1 + ((-1) << t);
        return v + vt;
    } else {
        return v;
    }
}

}

Jpeg::DataUnit Jpeg::readBlock(BitDecoder& dec, ImageComponentInScan& ic) {
    DataUnit du;
    du.fill(0);
    
    //first read the DC component. f.2.2.1
    dec.setTable(ic._tdTable);
    uint8_t t = dec.nextHuffmanByte();
    if (t > 15) throw std::runtime_error("syntax error, dc ssss great than 15");
    auto diffReceive = dec.nextXBits(t);
    ic.prevDC += ::extend_op(diffReceive, t);
    du[0] = ic.prevDC * (*ic._ic->_tqTable)[0];
    
    //read ac coefficients. f.2.2.2
    dec.setTable(ic._taTable);
    size_t k = 0;
    do {
        k++;
        uint8_t rs = dec.nextHuffmanByte();
        if (rs == 0x00) {
            //EOB. All remaining coefficients are zero.
            break;
        }
        else if (rs == 0xF0) {
            //ZRL. Skip 15 zero coefficients and this one is also zero.
            k += 16;
        } else {
            uint8_t r = rs >> 4;
            k += r;
            
            uint8_t ssss = rs & 0x0F;
            auto receive = dec.nextXBits(ssss);
            auto res = extend_op(receive, ssss);
            du[deZigZag(k)] = res * (*ic._ic->_tqTable)[k];
        }
    } while (k < 63);
    
    return du;
}

uint8_t Jpeg::deZigZag(uint8_t index) {
    const uint8_t zigzagTable[] = {
        0,   1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63,
    };
    
    assert(index < 64);
    
    return zigzagTable[index];
}

size_t Jpeg::readScanData(std::span<uint8_t> is) {
    size_t x = 0;
    size_t y = 0;
        
    BitDecoder dec;
    dec.setData(is);
    
    try {
        size_t position = 0;
        size_t restartInterval = _numberOfMCU;
        while (position < is.size()) {
            const size_t mcuRes = 16;
            
            readMCU(dec, x, y);
            restartInterval--;
                            
            x += mcuRes;
            if (x >= _x) {
                x = 0;
                y += mcuRes;
            }
            
            if (restartInterval == 0) {
                restartInterval = _numberOfMCU;
                dec.reset();
                for (auto& icS : _imageComponentsInScan) {
                    icS.prevDC = 0;
                }
            }
        }
    } catch (std::exception& e) {
        std::cout << "readScanData, exception: " << e.what() << std::endl;
    }
        
    copyImgCompToImage();
    ycbcrToRGB_accel(_metalDevice, _image, this->_x, this->_y);
    
    return dec.position();
}

void Jpeg::copyImgCompToImage() {
    auto commandQueue = NS::TransferPtr(_metalDevice->newCommandQueue());
    auto defaultLib = _metalDevice->newDefaultLibrary();
    auto function = defaultLib->newFunction(MTLSTR("copyLumaToImage"));
    NS::Error* error = nullptr;
    auto functionPSO = NS::TransferPtr(_metalDevice->newComputePipelineState(function, &error));
    
    auto bufferImage = NS::TransferPtr(_metalDevice->newBuffer(_image, _x * _y * sizeof(Colour), MTL::ResourceStorageModeShared, nullptr));
    
//    for (auto& ic : _imageComponents) {
        auto& ic = _imageComponents.front();
        
        
        auto icWidth = (_x / ic._hPixelsPerSample);
        auto icHeight = (_y / ic._vPixelsPerSample);
        
        auto bufferImgComponent = NS::TransferPtr(_metalDevice->newBuffer(ic._icSubPixelData.get(), icWidth * icHeight * sizeof(int), MTL::ResourceStorageModeShared, nullptr));

        auto commandBuffer = commandQueue->commandBuffer();
        auto computeEncoder = commandBuffer->computeCommandEncoder();
        computeEncoder->setComputePipelineState(functionPSO.get());
        computeEncoder->setBuffer(bufferImgComponent.get(), 0, 0);
        computeEncoder->setBuffer(bufferImage.get(), 0, 1);

        auto gridSize = MTL::Size(_x, _y, 1);
        auto threadGroupSizeObj = MTL::Size(16, 16, 1);
        
        computeEncoder->dispatchThreads(gridSize, threadGroupSizeObj);
        computeEncoder->endEncoding();
        
        commandBuffer->commit();
        commandBuffer->waitUntilCompleted();
//    }
}

void Jpeg::copyDUToSubpixels(DataUnit &du, image::Jpeg::ImageComponent &ic, size_t x, size_t y) {
    int* subpixelData = ic._icSubPixelData.get();
    size_t subpixelStart = y * _x + x;
    
    for (size_t duRow = 0; duRow < 8; duRow++) {
        size_t subpixelIncrement = duRow * (_x / ic._hPixelsPerSample);
        std::memcpy(&subpixelData[subpixelStart + subpixelIncrement], &du[duRow * 8], 8 * sizeof(DataUnit::value_type));
    }
}

void Jpeg::readMCU(BitDecoder& dec, size_t x, size_t y) {
    for (auto icIdx = 0; icIdx < _imageComponentsInScan.size(); icIdx++) {
        auto &icS = _imageComponentsInScan[icIdx];
                
        size_t duCount = icS._ic->_h * icS._ic->_v;
        
        if (duCount == 1) {
            auto du = readBlock(dec, icS);
            idct(du);
//            copyDUToSubpixels(du, *(icS._ic), x / icS._ic->_hPixelsPerSample, y / icS._ic->_vPixelsPerSample);
        } else {
            const std::array<size_t, 4> xOffsetForDU = {0, 8, 0, 8};
            const std::array<size_t, 4> yOffsetForDU = {0, 0, 8, 8};
            
            for (size_t i = 0; i < duCount; i++) {
                auto du = readBlock(dec, icS);
                idct(du);
                copyDUToSubpixels(du, *(icS._ic), x + xOffsetForDU[i], y + yOffsetForDU[i]);
            }
        }
    }
}

void Jpeg::appZeroData(std::span<uint8_t> data) {
    std::cout << "\tDecoding APP0 data per JFIF" << std::endl;
    _identifier.insert(_identifier.end(), data.begin(), data.begin() + 5);
    _version = *reinterpret_cast<uint16_t*>(&data[5]);
    _units = *reinterpret_cast<uint8_t*>(&data[7]);
}

void Jpeg::quantisationTable(std::span<uint8_t> data) {
    std::cout << "\tQuantisation Table data per B.2.4.1" << std::endl;
    
    uint16_t index = 0;
    while (index < data.size()) {
        uint8_t pq = data[index] >> 4; //quant precision flag
        uint8_t tq = data[index] & 0x0F; //quant table index
        index++;
        std::cout << "\tQuantisation table " << static_cast<int>(tq) << ", ";
        
        if (pq == 1) {
            std::cout << "precision is 16-bit" << std::endl;
            throw std::logic_error("not supported");
        } else if (pq == 0) {
            std::cout << "precision is 8-bit" << std::endl;
            std::copy(data.begin() + index, data.begin() + index + 64, _quantTables[tq].begin());
            index += 64;
        }
    }
}

void Jpeg::huffmanTable(std::span<uint8_t> data) {
    std::cout << "\n\tDHT Define Huffman table " << data.size() << " bytes";
    
    uint8_t tableClass = (*reinterpret_cast<uint8_t*>(&data[0]) & 0xF0) >> 4;
    uint8_t huffmanTableDestination = *reinterpret_cast<uint8_t*>(&data[0]) & 0x0F;
    std::cout << ", Table class (Tc): " << static_cast<int>(tableClass) << ", Table Destination (Th): " << static_cast<int>(huffmanTableDestination) << std::endl;
    
    std::span<uint8_t> tableDef(&data[1], data.size() - 1);
    HuffmanTable table = HuffmanTable::build(tableDef);
    
    if (tableClass == 0) {
        _huffmanTablesDC.at(huffmanTableDestination) = table;
    } else if (tableClass == 1) {
        _huffmanTablesAC.at(huffmanTableDestination) = table;
    }
}

void Jpeg::sofBaselineDCT(std::span<uint8_t> data) {
    std::cout << "\n\tSOF0 Baseline DCT Start of Frame" << std::endl;
    
    _frameSamplePrecision = *reinterpret_cast<uint8_t*>(&data[0]);
    _y = *reinterpret_cast<uint16_t*>(&data[1]);
    _y = htons(_y);
    _x = *reinterpret_cast<uint16_t*>(&data[3]);
    _x = htons(_x);
    uint8_t nf = *reinterpret_cast<uint8_t*>(&data[5]);
    
    _image = static_cast<Colour*>(malloc(_x * _y * sizeof(Colour)));
    
    for (unsigned int i = 0; i < nf; i++) {
        size_t byteStart = 6 + i * 3; //8 bits + 4 bits + 4 bits + 8 bits
        
        ImageComponent ic;
        ic._c = *reinterpret_cast<uint8_t*>(&data[byteStart + 0]);
        ic._h = *reinterpret_cast<uint8_t*>(&data[byteStart + 1]) >> 4;
        ic._v = *reinterpret_cast<uint8_t*>(&data[byteStart + 1]) & 0x0F;
        ic._tq = *reinterpret_cast<uint8_t*>(&data[byteStart + 2]);
        ic._tqTable = &_quantTables[(int)ic._tq];
        ic._icSubPixelData.reset(new int[(_x / ic._h) * (_y / ic._v)]);
        _imageComponents.push_back(std::move(ic));
        
        hMax = std::max(hMax, ic._h);
        vMax = std::max(vMax, ic._v);
        
        if (ic._h > _x) {
            throw std::logic_error("Image component horizontal sampling factor greater than max number of samples per line");
        }
        
        if (ic._v > _y) {
            throw std::logic_error("Image component vertical sampling factor greater than max number of lines");
        }
    }
    
    for (auto& ic : _imageComponents) {
        ic._hPixelsPerSample = hMax / ic._h;
        ic._vPixelsPerSample = vMax / ic._v;
    }
    
    if (nf != _imageComponents.size()) {
        std::logic_error("Less impact components specified than defined in frame");
    }
}

void Jpeg::startOfScan(std::span<uint8_t> data) {
    std::cout << "\n\tSOS Start of Scan" << std::endl;
    
    uint8_t ns = *reinterpret_cast<uint8_t*>(&data[0]);
    if (ns > _imageComponents.size()) {
        throw std::logic_error("More image components in scan than in frame.");
    }
    
    for (unsigned int j = 0; j < ns; j++) {
        size_t byteStart = 1 + 2 * j;
        
        ImageComponentInScan ic;
        ic._cs = *reinterpret_cast<uint8_t*>(&data[byteStart]);
        
        auto imgComponent = std::find_if(std::begin(_imageComponents), std::end(_imageComponents), [&ic] (const ImageComponent& el) { return ic._cs == el._c; } );
        if (imgComponent == std::end(_imageComponents)) {
            throw std::logic_error("Scan image component not found in frame");
        }
        ic._ic = &(*imgComponent);
        
        ic._td = *reinterpret_cast<uint8_t*>(&data[byteStart + 1]) >> 4;
        ic._ta = *reinterpret_cast<uint8_t*>(&data[byteStart + 1]) & 0x0F;
        
        ic._tdTable = &_huffmanTablesDC.at(ic._td);
        ic._taTable = &_huffmanTablesAC.at(ic._ta);
        _imageComponentsInScan.push_back(ic);
    }
    size_t afterComponents = 1 + 2 * ns;
    
    uint8_t ss = *reinterpret_cast<uint8_t*>(&data[afterComponents]);
    uint8_t se = *reinterpret_cast<uint8_t*>(&data[afterComponents + 1]);
    uint8_t ah = (*reinterpret_cast<uint8_t*>(&data[afterComponents + 2]) & 0xF0) >> 4;
    uint8_t al = *reinterpret_cast<uint8_t*>(&data[afterComponents + 2]) & 0x0F;
    
    if (ss != 0 || se != 63 || ah != 0 || al != 0) {
        throw std::logic_error("Start of scan end parameters not set for Sequential DCT");
    }
    
    _inScan = true;
}

void Jpeg::restartInterval(std::span<uint8_t> data) {
    if (data.size() != 2) {
        throw std::runtime_error("restart interval length is wrong");
    }
    
    _numberOfMCU = *reinterpret_cast<uint16_t*>(&data[0]);
    _numberOfMCU = htons(_numberOfMCU);
}

