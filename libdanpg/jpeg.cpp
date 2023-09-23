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

#include <arpa/inet.h>

using namespace image;

namespace {

template<typename T>
T getAs(void* data) {
    return *reinterpret_cast<T*>(data);
}

}

Jpeg::Jpeg(std::istream& is) {
    while (!is.eof()) {
        if (!_inScan) {
            readData(is);
        } else {
            readScanData(is);
        }
    }
}

Jpeg::Jpeg() {
    
}

void Jpeg::readData(std::istream& is) {
    uint8_t step = is.get();
    if (step == 0xff) {
        uint8_t markerCode = is.get();
        std::cout << "Marker : 0x" << std::hex << static_cast<int>(markerCode) << std::dec << " pos: " << is.tellg();// << std::endl;
        
        if (markerCode == 0xd8) {
            std::cout << "\tSOI Start of Image, skipping byte count" << std::endl;
            return;
        }
        
        uint16_t segmentBytes;
        is.read(reinterpret_cast<char*>(&segmentBytes), 2);
        segmentBytes = htons(segmentBytes);
        segmentBytes -= 2;
        
        std::cout << "\tSegment has " << segmentBytes << " bytes";
        
        std::vector<uint8_t> data;
        data.resize(segmentBytes);
        is.read(reinterpret_cast<char*>(&data[0]), data.size());
        
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
    }
}

namespace {

int extend(int v, int t) {
    auto vt = std::pow(2, t - 1);
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
    ic.prevDC += extend(diffReceive, t);
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
            auto res = extend(receive, ssss);
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

void Jpeg::readScanData(std::istream &is) {
    std::cout << "Reading scan data from position: " << is.tellg() << std::endl;
    
    size_t x = 0;
    size_t y = 0;
        
    BitDecoder dec;
    dec.setData(&is);
    
    try {
        while (is.peek() != std::istream::traits_type::eof()) {
            const size_t mcuRes = 16;
            std::vector<Colour> mcu(16*16);
            
            try {
                std::memset(&mcu[0], 0, sizeof(Colour) * 16 * 16);
                readMCU(dec, mcu);
                
                for (size_t line = 0; line < mcuRes; line++) {
                    size_t imageOffset = x + (y + line) * _x;
                    std::memcpy(&_image[imageOffset], &mcu[line * mcuRes], mcuRes * sizeof(Colour));
                }
                
                x += mcuRes;
                if (x >= _x) {
                    x = 0;
                    y += mcuRes;
                }
            } catch (ResetMarkerException& ex) {
                for (auto& icS : _imageComponentsInScan) {
                    icS.prevDC = 0;
                }
            }
        }
    } catch (std::exception& e) {
        std::cout << "readScanData, exception: " << e.what() << std::endl;
    }
}

namespace {

void copyDUIntoPixels(size_t mcuResolution, std::vector<Colour>& pixels, DataUnit& du, size_t subpixelIndex, image::Jpeg::ImageComponent& ic) {
    size_t pixel = 0;
    
    size_t yDU = 0;
    size_t yInc = 0;
    while (yDU < 8) {
        size_t xDU = 0;
        size_t xInc = 0;
        while (xDU < 8) {
            pixels.at(pixel).setIndexColour(subpixelIndex, du.at(yDU * 8 + xDU));
            
            pixel++;
            xInc++;
            if (xInc >= ic._hPixelsPerSample) {
                xDU++;
                xInc = 0;
            }
        }
        
        yInc++;
        if (yInc >= ic._vPixelsPerSample) {
            yDU++;
            yInc = 0;
        }
    }
}

void copy4DUIntoPixels(size_t mcuResolution, std::vector<Colour>& pixels, DataUnit& du, size_t subpixelIndex, image::Jpeg::ImageComponent& ic, size_t duIndex) {
    const std::array<size_t, 4> pixelStart = {0, 8, 128, 136};    
    size_t pixel = pixelStart[duIndex];
    
    size_t yDU = 0;
    while (yDU < 8) {
        size_t xDU = 0;
        while (xDU < 8) {
            pixels.at(pixel).setIndexColour(subpixelIndex, du.at(yDU * 8 + xDU));
            pixel++;
            xDU++;
        }
        
        pixel += 8;
        yDU++;
    }
}

}

void Jpeg::readMCU(BitDecoder& dec, std::vector<Colour>& mcu) {
    const size_t mcuResolution = 16;
    
    for (auto icIdx = 0; icIdx < _imageComponentsInScan.size(); icIdx++) {
        auto &icS = _imageComponentsInScan.at(icIdx);
                
        size_t duCount = icS._ic->_h * icS._ic->_v;
        
        if (duCount == 1) {
            auto du = idct(readBlock(dec, icS));
            copyDUIntoPixels(mcuResolution, mcu, du, icIdx, *(icS._ic));
        } else {
            for (size_t i = 0; i < duCount; i++) {
                auto du = idct(readBlock(dec, icS));
                copy4DUIntoPixels(mcuResolution, mcu, du, icIdx, *(icS._ic), i);
            }
        }
    }
        
    ycbcrToRGBInPlace(mcu);
}

void Jpeg::appZeroData(std::vector<uint8_t> &data) {
    std::cout << "\tDecoding APP0 data per JFIF" << std::endl;
    _identifier.insert(_identifier.end(), data.begin(), data.begin() + 5);
    _version = *reinterpret_cast<uint16_t*>(&data[5]);
    _units = *reinterpret_cast<uint8_t*>(&data[7]);
}

void Jpeg::quantisationTable(std::vector<uint8_t> &data) {
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

void Jpeg::huffmanTable(std::vector<uint8_t> &data) {
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

void Jpeg::sofBaselineDCT(std::vector<uint8_t> &data) {
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
        _imageComponents.push_back(ic);
        
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

void Jpeg::startOfScan(std::vector<uint8_t> &data) {
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

void Jpeg::restartInterval(std::vector<uint8_t> &data) {
    if (data.size() != 2) {
        throw std::runtime_error("restart interval length is wrong");
    }
    
    _numberOfMCU = *reinterpret_cast<uint16_t*>(&data[0]);
    _numberOfMCU = htons(_numberOfMCU);
}

