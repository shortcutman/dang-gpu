//
//  huffmantable.hpp
//  libdanpg
//
//  Created by Daniel Burke on 12/6/2023.
//

#ifndef huffmantable_hpp
#define huffmantable_hpp

#include <array>
#include <span>
#include <vector>
#include <cstdint>

class HuffmanTable {
public:
    HuffmanTable() {};
    
    std::array<uint8_t, 16> _bits;
    std::vector<uint8_t> _huffval;
    std::vector<uint8_t> _huffsize;
    std::vector<uint16_t> _huffcode;
    
    uint8_t decode(std::span<uint8_t> data);
    
    static HuffmanTable build(std::span<uint8_t> data);
};

class HuffmanDecoder {
private:
    HuffmanTable* _table;
    size_t _bits = 0;
    std::span<uint8_t> _data;
    
public:
    void setTable(HuffmanTable* table);
    void setData(std::span<uint8_t> data);
    
    uint8_t nextByte();
};

#endif /* huffmantable_hpp */
