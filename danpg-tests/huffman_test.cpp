//
//  huffman_test.cpp
//  danpg-tests
//
//  Created by Daniel Burke on 12/6/2023.
//

#include <gtest/gtest.h>
#include <string>

#include "huffmantable.hpp"

namespace {

TEST(Default, helloWorld) {
    const std::string a = "Hello World!";
    
    EXPECT_EQ(a, "Hello World!");
}

TEST(Huffman, ReadData) {
    std::vector<uint8_t> data;
    data.push_back('\0');
    data.push_back('\0');
    data.push_back('\x01');
    data.push_back('\x05');
    data.push_back('\x01');
    data.push_back('\x01');
    data.push_back('\x01');
    data.push_back('\x01');
    data.push_back('\x01');
    data.push_back('\x01');
    data.push_back('\0');
    data.push_back('\0');
    data.push_back('\0');
    data.push_back('\0');
    data.push_back('\0');
    data.push_back('\0');
    data.push_back('\0');
    data.push_back('\0');
    data.push_back('\x01');
    data.push_back('\x02');
    data.push_back('\x03');
    data.push_back('\x04');
    data.push_back('\x05');
    data.push_back('\x06');
    data.push_back('\a');
    data.push_back('\b');
    data.push_back('\t');
    data.push_back('\n');
    data.push_back('\v');
    
    auto table = HuffmanTable::build(data);
    
    std::array<uint8_t, 16> bitExpected = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_EQ(table._bits, bitExpected);
    
    std::vector<uint8_t> huffvalsExpected = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
    EXPECT_EQ(table._huffval, huffvalsExpected);
}

}
