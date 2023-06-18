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

#include <arpa/inet.h>

namespace {

template<typename T>
T getAs(void* data) {
    return *reinterpret_cast<T*>(data);
}

}

Jpeg::Jpeg(std::istream& is) {
    while (!is.eof()) {
        uint8_t step = is.get();
        if (step == 0xff) {
            uint8_t markerCode = is.get();
            std::cout << "Marker : 0x" << std::hex << static_cast<int>(markerCode) << std::dec << " pos: " << is.tellg();// << std::endl;
            
            if (markerCode == 0xd8) {
                std::cout << "\tSOI Start of Image, skipping byte count" << std::endl;
                continue;
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
    
    afterComponents += 3;
    std::span followingData{&data[afterComponents], data.size() - afterComponents };
}

void Jpeg::restartInterval(std::vector<uint8_t> &data) {
    if (data.size() != 2) {
        throw std::runtime_error("restart interval length is wrong");
    }
    
    _numberOfMCU = *reinterpret_cast<uint16_t*>(&data[0]);
    _numberOfMCU = htons(_numberOfMCU);
}

