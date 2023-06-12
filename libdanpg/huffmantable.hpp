//
//  huffmantable.hpp
//  libdanpg
//
//  Created by Daniel Burke on 12/6/2023.
//

#ifndef huffmantable_hpp
#define huffmantable_hpp

#include <array>
#include <vector>
#include <cstdint>

class HuffmanTable {
public:
    HuffmanTable() {};
    
    std::array<uint8_t, 16> _bits;
    std::vector<uint8_t> _huffval;
    std::vector<uint8_t> _huffsize;
    std::vector<uint16_t> _huffcode;
    
    uint8_t decode(const std::vector<uint8_t>& data);
    
    static HuffmanTable build(std::vector<uint8_t>& data);
};

#endif /* huffmantable_hpp */
