//
//  huffmantable.cpp
//  libdanpg
//
//  Created by Daniel Burke on 12/6/2023.
//

#include "huffmantable.hpp"

#include <sstream>
#include <cassert>

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

    size_t remainingListVals = 65536;
    size_t width = 65536;
    size_t huffcodesIndex = 0;
    auto* hufflistIdx = &table._hufflist[0];
    for (size_t bit = 1; bit <= 16; ++bit) {
        width /= 2;
        size_t bitCount = bits[bit - 1];
        if (!bitCount) continue;
        remainingListVals -= bitCount << (16 - bit);
        if (remainingListVals < 0 ) throw std::runtime_error("");
        for (size_t i = 0; i < bitCount; i++, huffcodesIndex++) {
            uint8_t huffval = table._huffval[huffcodesIndex];
            for (size_t j = 0; j < width; j++) {
                hufflistIdx->size = bit;
                hufflistIdx->val = huffval;
                ++hufflistIdx;
            }
        }
    }
    while (remainingListVals--) {
        hufflistIdx->size = 0;
        ++hufflistIdx;
    }
    
    return table;
}

void BitDecoder::setTable(HuffmanTable *table) {
    _table = table;
}

void BitDecoder::setData(std::span<uint8_t> data) {
    _data = data;
}

size_t BitDecoder::position() const {
    return _position;
}

void BitDecoder::reset() {
    _bitsIntoByte = 0;
    _bitsBuffered = 0;
    _currentBytes = 0;
    _markerBit.reset();
}

uint8_t BitDecoder::nextHuffmanByte() {
    uint16_t potentialCode = peakXBits(16);
    auto entry = _table->_hufflist[potentialCode];
    if (entry.size == 0) {
        throw std::runtime_error("huffman error");
    }
    
    nextXBits(entry.size);
    return entry.val;
}

uint16_t BitDecoder::nextXBits(size_t bits) {
    if (bits > 16) {
        throw std::logic_error("Advancing by more than 16 bits not supported");
    }
    
    bufferBits(bits, true);
    
    if (bits > _bitsBuffered) {
        throw std::runtime_error("Not enough bytes for request");
    }
    
    uint16_t requestedBits = _currentBytes >> (_bitsBuffered - bits);
    _currentBytes = static_cast<uint32_t>(static_cast<uint64_t>(_currentBytes) << (32 - _bitsBuffered + bits));
    _currentBytes = _currentBytes >> (32 - (_bitsBuffered - bits));
    _bitsBuffered -= bits;
    return requestedBits;
}

uint16_t BitDecoder::peakXBits(size_t bits) {
    if (bits > 16) {
        throw std::logic_error("Advancing by more than 16 bits not supported");
    }
    
    bufferBits(bits, false);
    
    if (_bitsBuffered > bits) {
        uint16_t requestedBits = _currentBytes >> (_bitsBuffered - bits);
        return requestedBits;
    } else {
        uint16_t requestedBits = _currentBytes << (bits - _bitsBuffered);
        return requestedBits;
    }
}

void BitDecoder::bufferBits(size_t bits, bool reading) {
    if (bits > 16) {
        throw std::logic_error("Advancing by more than 16 bits not supported");
    }
    
    if (_markerBit) {
        if (bits > *_markerBit) {
            if (reading) {
//                throw ResetMarkerException();
                return;
            } else {
                return;
            }
        } else if (reading) {
            _markerBit = *_markerBit - bits;
        }
    }
    
    while (_bitsBuffered < bits && _position < _data.size()) {
        auto nextByte = _data[_position++];
        if (nextByte == 0xFF) {
            if (_position >= _data.size()) {
                throw std::runtime_error("Marker segment with no other byte");
            }
            
            auto peekByte = _data[_position];
            if (peekByte == 0x00) {
                _position++; //byte stuffing, F.1.2.3. throw this away.
            } else if (peekByte >= 0xD0 && peekByte <= 0xD7) {
                //restart marker, consume marker, reset own state, and signal caller to reset
                _position++;
                _markerBit = _bitsBuffered;
                break;
            } else {
                std::stringstream ss;
                ss << "Marker: " << std::hex << 0xFF << peekByte;
                throw std::runtime_error(ss.str());
            }
        }
        
        _currentBytes <<= 8;
        _currentBytes |= nextByte;
        _bitsBuffered += 8;
    }
}
