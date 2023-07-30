//
//  huffmantable.cpp
//  libdanpg
//
//  Created by Daniel Burke on 12/6/2023.
//

#include "huffmantable.hpp"

#include <iostream>
#include <map>

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

void BitDecoder::setData(std::span<uint8_t> data) {
    _data = data;
}

size_t BitDecoder::offset() const {
    return _offset;
}

uint8_t BitDecoder::nextByte() {
    uint16_t potentialCode = getNext16bits();
    uint8_t numberOfBits = _table->_huffsize.front();
    uint16_t mask = 0xFFFF << (16 - numberOfBits);
        
    for (size_t i = 0; i < _table->_huffcode.size(); i++) {
        if (_table->_huffsize[i] > numberOfBits) {
            mask = 0xFFFF << (16 - _table->_huffsize[i]);
            numberOfBits = _table->_huffsize[i];
        }
        
        uint16_t codeEntry = (_table->_huffcode[i] << (16 - numberOfBits));
        
        if (codeEntry == (potentialCode & mask)) {
            advanceBits(numberOfBits);
            return _table->_huffval[i];
        }
    }
    
    throw std::runtime_error("No code found.");
    return 0;
}

uint16_t BitDecoder::nextXBits(size_t bits) {
    uint16_t value = getNext16bits();
    uint16_t mask = 0xFFFF << (16 - bits);
    advanceBits(bits);
    return (value & mask) >> (16 - bits);
}

uint16_t BitDecoder::getNext16bits() {
    uint16_t next16 = _data[_offset] << (8 + _bits);
    
    if ((next16 & 0xFF00) == 0xFF00) {
        if (_data.size() > 1) {
            if (_data[_offset + 1] == 0x00) {
                _offset++; //skip byte stuffing
            } else {
                throw std::runtime_error("Encountered marker segment");
            }
        }
    }
    
    if (_data.size() >  1) {
        next16 += _data[_offset + 1] << _bits;
        if (_data.size() > 2 && _bits > 0) {
            next16 += _data[_offset + 2] >> (8 - _bits);
        }
    }
    
    return next16;
}

void BitDecoder::advanceBits(uint8_t bits) {
    _bits += bits;
    if (_bits >= 8) {
        _offset++;
        _bits -= 8;
    }
}
