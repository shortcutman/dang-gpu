//
//  huffmantable.cpp
//  libdanpg
//
//  Created by Daniel Burke on 12/6/2023.
//

#include "huffmantable.hpp"

#include <iostream>
#include <map>

HuffmanTable HuffmanTable::build(std::vector<uint8_t> &data) {
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
