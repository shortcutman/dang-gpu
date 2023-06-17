//
//  huffmantable.cpp
//  libdanpg
//
//  Created by Daniel Burke on 12/6/2023.
//

#include "huffmantable.hpp"

#include <iostream>
#include <map>

uint8_t HuffmanTable::decode(std::span<uint8_t> data) {
    uint16_t word = 0;
    uint8_t byte = data[0];
    
    uint8_t numberOfBits = _huffsize.front();
    uint16_t mask = 0xFFFF << (16 - numberOfBits);
    
    word = byte;
    word = word << 8;
    
    for (size_t i = 0; i < _huffcode.size(); i++) {
        if (_huffsize[i] > numberOfBits) {
            mask = 0xFFFF << (16 - _huffsize[i]);
            numberOfBits = _huffsize[i];
        }
        
        if ((_huffcode[i] << (16 - numberOfBits)) == (word & mask)) {
            return _huffval[i];
        }
    }
    
    return 0;
}

HuffmanTable HuffmanTable::build(std::span<uint8_t> data) {
    size_t codeCount = 0;
    std::array<uint8_t, 16> bits;
    for (unsigned int i = 0; i < bits.size(); i++) {
        bits[i] = *reinterpret_cast<uint8_t*>(&data[1 + i]);
        codeCount += bits[i];
    }
    
    std::vector<uint8_t> huffval;
    for (unsigned int i = 0; i < codeCount; i++) {
        huffval.push_back(*reinterpret_cast<uint8_t*>(&data[17 + i]));
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

void HuffmanDecoder::setTable(HuffmanTable *table) {
    _table = table;
}

void HuffmanDecoder::setData(std::span<uint8_t> data) {
    _data = data;
}

uint8_t HuffmanDecoder::nextByte() {
    uint16_t potentialCode = _data[0] << (8 + _bits);
    if (_data.size() >  1) {
        potentialCode += _data[1] << _bits;
        if (_data.size() > 2) {
            potentialCode += _data[2] << _bits;
        }
    }
    
    uint8_t numberOfBits = _table->_huffsize.front();
    uint16_t mask = 0xFFFF << (16 - numberOfBits);
        
    for (size_t i = 0; i < _table->_huffcode.size(); i++) {
        if (_table->_huffsize[i] > numberOfBits) {
            mask = 0xFFFF << (16 - _table->_huffsize[i]);
            numberOfBits = _table->_huffsize[i];
        }
        
        if ((_table->_huffcode[i] << (16 - numberOfBits)) == (potentialCode & mask)) {
            _bits += numberOfBits;
            if (_bits >= 8) {
                _data = _data.subspan(1);
                _bits -= 8;
            }
            
            return _table->_huffval[i];
        }
    }
    
    return 0;
}
