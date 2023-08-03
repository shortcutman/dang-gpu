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
#include <istream>

class HuffmanTable {
public:
    HuffmanTable() {};
    
    std::array<uint8_t, 16> _bits;
    std::vector<uint8_t> _huffval;
    std::vector<uint8_t> _huffsize;
    std::vector<uint16_t> _huffcode;
        
    static HuffmanTable build(std::span<uint8_t> data);
};

class BitDecoder {
private:
    HuffmanTable* _table = nullptr;
    uint8_t _bits = 0;
    std::istream* _data = nullptr;
    uint32_t _currentBytes = 0;
    
public:
    void setTable(HuffmanTable* table);
    void setData(std::istream* data);
    
    uint8_t nextByte();
    uint16_t nextXBits(size_t bits);

private:
    uint16_t getNext16bits();
    void advanceBits(uint8_t bits);
};

#endif /* huffmantable_hpp */
