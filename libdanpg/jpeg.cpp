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
    uint8_t t = dec.nextByte();
    if (t > 15) throw std::runtime_error("syntax error, dc ssss great than 15");
    auto diffReceive = dec.nextXBits(t);
    ic.prevDC += extend(diffReceive, t);
    du[0] = ic.prevDC;
    
    //read ac coefficients. f.2.2.2
    dec.setTable(ic._taTable);
    size_t k = 0;
    do {
        k++;
        uint8_t rs = dec.nextByte();
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
            du[k] = res;
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

Jpeg::DataUnit Jpeg::dequantiseBlock(DataUnit du, ImageComponent ic) {
    DataUnit out;
    for (uint8_t i = 0; i < du.size(); i++) {
        out[deZigZag(i)] = (int)du[i] * (*ic._tqTable)[i];
    }
    
    return out;
}

void Jpeg::readScanData(std::istream &is) {
    std::cout << "Reading scan data from position: " << is.tellg() << std::endl;
    
    std::vector<Colour> image(_x * _y, {0, 0, 0});
    
    _x = 32;
    _y = 16;
    
    BitDecoder dec;
    dec.setData(&is);
    
    try {
        size_t x = 0;
        size_t y = 0;
        
//        while (is.peek() != std::istream::traits_type::eof()) {
        while (x < _x && y < _y) {
            const size_t mcuRes = 16;
            auto mcu = readMCU(dec);
            
            for (size_t line = 0; line < mcuRes; line++) {
                size_t imageOffset = x + (y + line) * _x;
                std::memcpy(&image[imageOffset], &mcu[line * mcuRes], mcuRes * sizeof(Colour));
            }
            
            x += mcuRes;
            if (x >= _x) {
                x = 0;
                y += mcuRes;
            }
        }
    } catch (std::exception& e) {
        std::cout << "readScanData, exception: " << e.what() << std::endl;
    }
    
    writeOutPPM("/private/tmp/jpeg.ppm", _x, _y, image);
    
    abort();
}

std::vector<Colour> Jpeg::readMCU(BitDecoder& dec) {
    std::vector<std::tuple<ImageComponent, std::vector<DataUnit>>> duMap;
    size_t mcuResolution = 0;
    
    for (auto& icS : _imageComponentsInScan) {
        auto icIt = std::find_if(_imageComponents.begin(), _imageComponents.end(), [&icS] (auto t) { return icS._cs == t._c;});
        if (icIt == _imageComponents.end()) {
            throw std::runtime_error("Image Component doesn't exist");
        }
        
        size_t duCount = icIt->_h * icIt->_v;
        mcuResolution = std::max(mcuResolution, duCount / 2 * 8);
        std::vector<DataUnit> icDUs;
        
        for (size_t i = 0; i < duCount; i++) {
            auto du = idct(dequantiseBlock(readBlock(dec, icS), *icIt));
            icDUs.push_back(du);
        }
        duMap.push_back(std::make_tuple(*icIt, icDUs));
    }
    
    const uint8_t hMax = 2;
    const uint8_t vMax = 2;
    
    std::vector<Colour> pixels(mcuResolution * mcuResolution);
    for (size_t y = 0; y < mcuResolution; y++) {
        for (size_t x = 0; x < mcuResolution; x++) {
            for (size_t icIdx = 0; icIdx < duMap.size(); icIdx++) {
                auto ic  = std::get<0>(duMap.at(icIdx));
                size_t xScale = mcuResolution / ic._h;
                size_t yScale = mcuResolution / ic._v;
                
                auto icDU = std::get<1>(duMap.at(icIdx)).at(x / xScale + (y / yScale) * ic._v);
                auto icDUx = (x / (hMax / ic._h)) % 8;
                auto icDUy = (y / (vMax / ic._v)) % 8;
                pixels.at(x + y * mcuResolution).at(icIdx) = icDU.at(icDUx + icDUy * 8);
            }
        }
    }
    
    ycbcrToRGBInPlace(pixels);
    
    return pixels;
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
    
    for (unsigned int i = 0; i < nf; i++) {
        size_t byteStart = 6 + i * 3; //8 bits + 4 bits + 4 bits + 8 bits
        
        ImageComponent ic;
        ic._c = *reinterpret_cast<uint8_t*>(&data[byteStart + 0]);
        ic._h = *reinterpret_cast<uint8_t*>(&data[byteStart + 1]) >> 4;
        ic._v = *reinterpret_cast<uint8_t*>(&data[byteStart + 1]) & 0x0F;
        ic._tq = *reinterpret_cast<uint8_t*>(&data[byteStart + 2]);
        ic._tqTable = &_quantTables[(int)ic._tq];
        _imageComponents.push_back(ic);
        
        if (ic._h > _x) {
            throw std::logic_error("Image component horizontal sampling factor greater than max number of samples per line");
        }
        
        if (ic._v > _y) {
            throw std::logic_error("Image component vertical sampling factor greater than max number of lines");
        }
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
        
        if (std::find_if(std::begin(_imageComponents), std::end(_imageComponents), [&ic] (const ImageComponent& el) { return ic._cs == el._c; } ) == std::end(_imageComponents)) {
            throw std::logic_error("Scan image component not found in frame");
        }
        
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

