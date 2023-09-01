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

class ResetMarkerException : public std::exception {
    
};

class BitDecoder {
private:
    HuffmanTable* _table = nullptr;
    std::istream* _data = nullptr;
    
    uint32_t _bitsIntoByte = 0;
    uint32_t _bitsBuffered = 0;
    uint32_t _currentBytes = 0;
    
public:
    void setTable(HuffmanTable* table);
    void setData(std::istream* data);
    
    uint8_t nextHuffmanByte();
    uint16_t nextXBits(size_t bits);
};

#endif /* huffmantable_hpp */
