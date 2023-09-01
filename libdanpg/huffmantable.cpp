//
//  huffmantable.cpp
//  libdanpg
//
//  Created by Daniel Burke on 12/6/2023.
//

#include "huffmantable.hpp"

#include <iostream>
#include <map>
#include <sstream>

HuffmanTable HuffmanTable::build(std::span<uint8_t> data) {
    size_t codeCount = 0;
    std::array<uint8_t, 16> bits;
    for (unsigned int i = 0; i < bits.size(); i++) {
        bits[i] = *reinterpret_cast<uint8_t*>(&data[i]);
        codeCount += bits[i];
    }
    
    std::vector<uint8_t> huffval;
    for (unsigned int i = 0; i < codeCount; i++) {
        huffval.push_back(*reinterpret_cast<uint8_t*>(&data[16 + i]));
    }
    
    std::vector<uint8_t> huffsize;
    {
        unsigned int i = 1;
        do {
            unsigned int j = 1;
            while (j <= bits.at(i - 1)) {
                huffsize.push_back(i);
                j++;
            }
            i++;
        } while (i <= 16);
        huffsize.push_back(0);
    }
    
    std::vector<uint16_t> huffcode;
    {
        size_t k = 0;
        uint16_t code = 0;
        size_t si = huffsize[0]; //huffman code size
        
        for (;;) {
            do {
                huffcode.push_back(code);
                code++;
                k++;
            } while (huffsize[k] == si);
            
            if (huffsize[k] == 0) {
                break;
            }
            
            do {
                code = code << 1;
                si++;
            } while (huffsize[k] != si);
        }
    }
    
    HuffmanTable table;
    table._bits = bits;
    table._huffval = huffval;
    table._huffsize = huffsize;
    table._huffcode = huffcode;
    return table;
}

void BitDecoder::setTable(HuffmanTable *table) {
    _table = table;
}

void BitDecoder::setData(std::istream* data) {
    _data = data;
}

uint8_t BitDecoder::nextHuffmanByte() {
    uint8_t numberOfBits = 0;
    uint16_t potentialCode = 0;
    for (size_t i = 0; i < _table->_huffcode.size(); i++) {
        if (_table->_huffsize[i] > numberOfBits) {
            uint8_t numberOfNewBits = _table->_huffsize[i] - numberOfBits;
            numberOfBits = _table->_huffsize[i];
            
            potentialCode <<= numberOfNewBits;
            potentialCode |= nextXBits(numberOfNewBits);
        }
        
        uint16_t codeEntry = _table->_huffcode[i];
        
        if (codeEntry == potentialCode) {
            return _table->_huffval[i];
        }
    }
    
    throw std::runtime_error("No code found.");
    return 0;
}

uint16_t BitDecoder::nextXBits(size_t bits) {
    if (bits > 16) {
        throw std::logic_error("Advancing by more than 16 bits not supported");
    }
    
    while (_bitsBuffered < bits) {
        auto nextByte = _data->get();
        if (nextByte == std::istream::traits_type::eof()) {
            throw std::runtime_error("Not enough bytes for request");
        } else if (nextByte == 0xFF) {
            auto peekByte = _data->peek();
            if (peekByte != 0x00) {
                std::stringstream ss;
                ss << "Marker: " << std::hex << 0xFF << peekByte;
                throw std::runtime_error(ss.str());
            } else {
                _data->get();
            }
        }
        
        _currentBytes <<= 8;
        _currentBytes |= nextByte;
        _bitsBuffered += 8;
    }
    
    uint16_t requestedBits = _currentBytes >> (_bitsBuffered - bits);
    _currentBytes = static_cast<uint32_t>(static_cast<uint64_t>(_currentBytes) << (32 - _bitsBuffered + bits));
    _currentBytes = _currentBytes >> (32 - (_bitsBuffered - bits));
    _bitsBuffered -= bits;
    return requestedBits;
}
