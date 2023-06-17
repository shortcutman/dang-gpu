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
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    
    auto table = HuffmanTable::build(data);
    
    std::array<uint8_t, 16> bitExpected = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_EQ(table._bits, bitExpected);
    
    std::vector<uint8_t> huffvalsExpected = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
    EXPECT_EQ(table._huffval, huffvalsExpected);
}

TEST(Huffman, CalculateData) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    
    auto table = HuffmanTable::build(data);
    
    std::array<uint8_t, 16> bitExpected = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_EQ(table._bits, bitExpected);
    
    std::vector<uint8_t> huffsizeExpected = {0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00};
    EXPECT_EQ(table._huffsize, huffsizeExpected);
    
    std::vector<uint16_t> huffcodeExpected = {0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0e, 0x1e, 0x3e, 0x7e, 0xfe, 0x1fe};
    EXPECT_EQ(table._huffcode, huffcodeExpected);
}

TEST(Huffman, Decode2bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    
    std::vector<uint8_t> encoded = {0x00};
    auto byte = table.decode(encoded);
    EXPECT_EQ(byte, 0x00);
}

TEST(Huffman, Decode3bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    
    std::vector<uint8_t> encoded = {0x80};
    auto byte = table.decode(encoded);
    EXPECT_EQ(byte, 0x03);
}

TEST(HuffmanDecoder, Decode2bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    HuffmanDecoder decoder;
    decoder.setTable(&table);
    
    std::vector<uint8_t> encoded = {0x00};
    decoder.setData(encoded);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x00);
}

TEST(HuffmanDecoder, Decode3bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    HuffmanDecoder decoder;
    decoder.setTable(&table);
    
    std::vector<uint8_t> encoded = {0x80};
    decoder.setData(encoded);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x03);
}

TEST(HuffmanDecoder, Decode4bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    HuffmanDecoder decoder;
    decoder.setTable(&table);
    
    std::vector<uint8_t> encoded = {0xE0};
    decoder.setData(encoded);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x06);
}

TEST(HuffmanDecoder, Decode5bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    HuffmanDecoder decoder;
    decoder.setTable(&table);
    
    std::vector<uint8_t> encoded = {0xF0};
    decoder.setData(encoded);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x07);
}

TEST(HuffmanDecoder, Decode6bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    HuffmanDecoder decoder;
    decoder.setTable(&table);
    
    std::vector<uint8_t> encoded = {0xF8};
    decoder.setData(encoded);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x08);
}

TEST(HuffmanDecoder, Decode7bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    HuffmanDecoder decoder;
    decoder.setTable(&table);
    
    std::vector<uint8_t> encoded = {0xFC};
    decoder.setData(encoded);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x09);
}

TEST(HuffmanDecoder, Decode8bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    HuffmanDecoder decoder;
    decoder.setTable(&table);
    
    std::vector<uint8_t> encoded = {0xFE};
    decoder.setData(encoded);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x0a);
}

TEST(HuffmanDecoder, Decode9bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    HuffmanDecoder decoder;
    decoder.setTable(&table);
    
    std::vector<uint8_t> encoded = {0xFF, 0x00};
    decoder.setData(encoded);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x0b);
}

TEST(HuffmanDecoder, DecodeSubsequent3bitCodes) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    HuffmanDecoder decoder;
    decoder.setTable(&table);
    
    std::vector<uint8_t> encoded = {0x94};// code b100: val 0x03, code b101 : val 0x4, b00 padding
    decoder.setData(encoded);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x03);
    
    byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x04);
}

TEST(HuffmanDecoder, Decode3bitThen9bit) {
    std::vector<uint8_t> data = {'\0', '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a', '\b', '\t', '\n', '\v'};
    HuffmanTable table = HuffmanTable::build(data);
    HuffmanDecoder decoder;
    decoder.setTable(&table);
    
    // code b100: val 0x03, code b1 1111 1110 : val 0xb, b0000 padding
    std::vector<uint8_t> encoded = {0x9F, 0xE0};
    decoder.setData(encoded);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x03);
    
    byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x0b);
}

}
