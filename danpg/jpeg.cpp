//
//  jpeg.cpp
//  danpg
//
//  Created by Daniel Burke on 2/5/2023.
//

#include "jpeg.hpp"

#include <cstddef>
#include <iostream>
#include <bit>
#include <map>
#include <fstream>
#include <numbers>
#include <cmath>

#include <arpa/inet.h>

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

Jpeg::DataUnit Jpeg::readBlock(HuffmanDecoder& dec, ImageComponentInScan ic, bool resetDC) {
    static int prevDC = 0;
    
    if (resetDC) {
        prevDC = 0;
    }
    
    DataUnit du;
    du.fill(0);
    
    //first read the DC component. f.2.2.1
    dec.setTable(&_huffmanTablesDC.at(ic._td));
    uint8_t t = dec.nextByte();
    if (t > 15) throw std::runtime_error("syntax error, dc ssss great than 15");
    auto diffReceive = dec.nextXBits(t);
    prevDC += extend(diffReceive, t);
    du[0] = prevDC;
    
    //read ac coefficients. f.2.2.2
    dec.setTable(&_huffmanTablesAC.at(ic._ta));
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
            std::cout << "res: " << static_cast<int>(res) << std::endl;
//            uint8_t zz = deZigZag(k);
//            du[zz] = res;
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
        out[deZigZag(i)] = (int)du[i] * _quantTables[(int)ic._tq][i];
    }
    
    return out;
}

Jpeg::DataUnit Jpeg::idct(DataUnit du) {
    DataUnit out;
    
    for (size_t y = 0; y < 8; y++) {
        for (size_t x = 0; x < 8; x++) {
            float val = 0.f;
            
            for (size_t u = 0; u < 8; u++) {
                for (size_t v = 0; v < 8; v++) {
                    float cu = (u == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    float cv = (v == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    val += cu * cv * du[u + v*8]
                            * std::cos((2.f * x + 1.f) * u * std::numbers::pi / 16.f)
                            * std::cos((2.f * y + 1.f) * v * std::numbers::pi / 16.f);
                }
            }
            val *= 0.25f;
            
            out[x + y * 8] = static_cast<int>(val);
        }
    }
    
    return out;
}

int adjustAndClamp(float val) {
    val += 128.f;
    
    if (val > 255) {
        return 255;
    } else if (val < 0) {
        return 0;
    } else {
        return static_cast<int>(val);
    }
}

void Jpeg::readScanData(std::istream &is) {
    std::cout << "Reading scan data from position: " << is.tellg() << std::endl;
    
    std::vector<uint8_t> data;
    data.resize(2048);
    is.read(reinterpret_cast<char*>(&data[0]), data.size());

    HuffmanDecoder dec;
    dec.setData(data);
    //read data unit 1, 2, 3, 4 of image component 1
    auto ic = _imageComponentsInScan.at(0);
    if (ic._cs != 1) throw std::runtime_error("wrong image component");
    
    std::vector<DataUnit> ic1;

    ic1.push_back(idct(dequantiseBlock(readBlock(dec, ic, false), _imageComponents.at(0))));
    ic1.push_back(idct(dequantiseBlock(readBlock(dec, ic, false), _imageComponents.at(0))));
    ic1.push_back(idct(dequantiseBlock(readBlock(dec, ic, false), _imageComponents.at(0))));
    ic1.push_back(idct(dequantiseBlock(readBlock(dec, ic, false), _imageComponents.at(0))));
    
    auto ic20 = idct(dequantiseBlock(readBlock(dec, _imageComponentsInScan.at(1), true), _imageComponents.at(1)));
    
    auto ic30 = idct(dequantiseBlock(readBlock(dec, _imageComponentsInScan.at(2), true), _imageComponents.at(2)));
    
    std::array<Colour, 16*16> mcuYCbCr;
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 16; x++) {
            auto ic1du = ic1.at(x / 8 + (y / 8)*2);
            auto ic1x = x % 8;
            auto ic1y = y % 8;
            auto luma = ic1du.at(ic1x + ic1y * 8);
            
            auto ic2x = x / 2;
            auto ic2y = y / 2;
            auto cb = ic20.at(ic2x + ic2y * 8);
            
            auto ic3x = x / 2;
            auto ic3y = y / 2;
            auto cr = ic30.at(ic3x + ic3y * 8);
            
            mcuYCbCr[x + y*16] = std::make_tuple(luma, cb, cr);
        }
    }
    
    std::array<Colour, 16*16> mcuRGB;
    for (size_t i = 0; i < mcuYCbCr.size(); i++) {
        Colour ybr = mcuYCbCr[i];
        float y = std::get<0>(mcuYCbCr[i]);
        float cb = std::get<1>(mcuYCbCr[i]);
        float cr = std::get<2>(mcuYCbCr[i]);
        auto r = adjustAndClamp(y + (1.402f * cr));
        auto g = adjustAndClamp(y - (0.34414f * cb) - (0.71414f * cr));
        auto b = adjustAndClamp(y + (1.772f * cb));
        mcuRGB[i] = std::make_tuple(r, g, b);
    }
    
    std::ofstream file;
    file.open("/tmp/jpeg.ppm");
    
    if (!file.is_open()) {
        abort();
    }
    
    file << "P3" << std::endl;
    file << "16" << " " << "16" << std::endl;
    file << "255" << std::endl;
     
     for (size_t y = 0; y < 16; y++) {
         for (size_t x = 0; x < 16; x++) {
             auto pixel = mcuRGB[x + y * 16];
             
             file << std::to_string(std::get<0>(pixel)) << " "
                  << std::to_string(std::get<1>(pixel)) << " "
                  << std::to_string(std::get<2>(pixel)) << " ";
         }
         
         file << std::endl;
     }
    
    file.close();
    
    abort();
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

