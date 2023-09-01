//
//  huffman_test.cpp
//  danpg-tests
//
//  Created by Daniel Burke on 12/6/2023.
//

#include <gtest/gtest.h>
#include <string>
#include <strstream>

#include "huffmantable.hpp"

namespace {

TEST(HuffmanTable, ReadData) {
    std::vector<uint8_t> data = {
        '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01',
        '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
        '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a',
        '\b', '\t', '\n', '\v'};
    
    auto table = HuffmanTable::build(data);
    
    std::array<uint8_t, 16> bitExpected = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_EQ(table._bits, bitExpected);
    
    std::vector<uint8_t> huffvalsExpected = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
    EXPECT_EQ(table._huffval, huffvalsExpected);
}

TEST(HuffmanTable, CalculateData) {
    std::vector<uint8_t> data = {
        '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01',
        '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
        '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a',
        '\b', '\t', '\n', '\v'};
    
    auto table = HuffmanTable::build(data);
    
    std::array<uint8_t, 16> bitExpected = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_EQ(table._bits, bitExpected);
    
    std::vector<uint8_t> huffsizeExpected = {0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00};
    EXPECT_EQ(table._huffsize, huffsizeExpected);
    
    std::vector<uint16_t> huffcodeExpected = {0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0e, 0x1e, 0x3e, 0x7e, 0xfe, 0x1fe};
    EXPECT_EQ(table._huffcode, huffcodeExpected);
}

class HuffmanDecoderTest : public ::testing::Test {
protected:
    void SetUp() override {
        decoder.setTable(&table);
    }
    
    std::vector<uint8_t> data = {
        '\0', '\x01', '\x05', '\x01', '\x01', '\x01', '\x01', '\x01',
        '\x01', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
        '\0', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\a',
        '\b', '\t', '\n', '\v'
    };
    HuffmanTable table = HuffmanTable::build(data);
    BitDecoder decoder;
};

TEST_F(HuffmanDecoderTest, Decode2bit) {
    std::vector<uint8_t> encoded = {0x00};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x00);
}

TEST_F(HuffmanDecoderTest, Decode3bit) {
    std::vector<uint8_t> encoded = {0x80};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x03);
}

TEST_F(HuffmanDecoderTest, Decode4bit) {
    std::vector<uint8_t> encoded = {0xE0};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x06);
}

TEST_F(HuffmanDecoderTest, Decode5bit) {
    std::vector<uint8_t> encoded = {0xF0};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x07);
}

TEST_F(HuffmanDecoderTest, Decode6bit) {
    std::vector<uint8_t> encoded = {0xF8};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x08);
}

TEST_F(HuffmanDecoderTest, Decode7bit) {
    std::vector<uint8_t> encoded = {0xFC};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x09);
}

TEST_F(HuffmanDecoderTest, Decode8bit) {
    std::vector<uint8_t> encoded = {0xFE};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x0a);
}

TEST_F(HuffmanDecoderTest, Decode9bit) {
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x00};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextXBits(9);
    EXPECT_EQ(byte, 0x1FE);
}

TEST_F(HuffmanDecoderTest, HuffmanDecode9bit) {
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x00};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x0b);
}

TEST_F(HuffmanDecoderTest, Decode3bitThen3bit) {
    // code b100: val 0x03, code b101 : val 0x4, b00 padding
    std::vector<uint8_t> encoded = {0x94};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0b100);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0b101);
}

TEST_F(HuffmanDecoderTest, HuffmanDecode3bitThen3bit) {
    // code b100: val 0x03, code b101 : val 0x4, b00 padding
    std::vector<uint8_t> encoded = {0x94};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x03);
    
    byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x04);
}

TEST_F(HuffmanDecoderTest, Decode8bitThen3bit) {
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x00};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextXBits(8);
    EXPECT_EQ(byte, 0xFF);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0x00);
}

TEST_F(HuffmanDecoderTest, Decode3bitThen5bitThen3bit) {
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x00};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0b111);
    
    byte = decoder.nextXBits(5);
    EXPECT_EQ(byte, 0b11111);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0x00);
}

TEST_F(HuffmanDecoderTest, Decode3bitThen9bit) {
    // code b100: val 0x03, code b1 1111 1110 : val 0xb, b0000 padding
    std::vector<uint8_t> encoded = {0x9F, 0xE0};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x03);
    
    byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x0b);
}

TEST_F(HuffmanDecoderTest, Decode9bitThen8bitThen3bit) {
    // code b1 1111 1110 : val 0xb, code b100: val 0x03, b0000 padding
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x40};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextXBits(9);
    EXPECT_EQ(byte, 0x1fe);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0x04);
}

TEST_F(HuffmanDecoderTest, HuffmanDecode9bitThen3bit) {
    // code b1 1111 1110 : val 0xb, code b100: val 0x03, b0000 padding
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x40};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x0b);
    
    byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x03);
}

TEST_F(HuffmanDecoderTest, Decode9bitThen9bit) {
    // code b1 1111 1110 : val 0xb, code b1 1111 1110 : val 0xb, b00 0000 1111 1111 padding
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x7F, 0x80};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x0b);
    
    byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x0b);
}

TEST_F(HuffmanDecoderTest, Decode6bitThen9bit) {
    // code b111110: val 0x08, code b1 1111 1110 : val 0xb, b00 0000 0000 padding
    std::vector<uint8_t> encoded = {0xFB, 0xFC, 0x00};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x08);
    
    byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x0b);
}

TEST_F(HuffmanDecoderTest, DecodeMarkerSegmentException) {
    std::vector<uint8_t> encoded = {0xFF, 0xD0};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    EXPECT_THROW(decoder.nextByte(), std::runtime_error);
}

TEST_F(HuffmanDecoderTest, NotEnoughBytesErrorDueToNoBytes) {
    std::vector<uint8_t> encoded = {0xFF};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    
    EXPECT_THROW(decoder.nextXBits(9), std::runtime_error);
}

TEST_F(HuffmanDecoderTest, NotEnoughBytesErrorDueToMarkerSegment) {
    std::vector<uint8_t> encoded = {0xFF, 0x00};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    
    EXPECT_THROW(decoder.nextXBits(9), std::runtime_error);
}

TEST_F(HuffmanDecoderTest, Decode3bitSkip3Then3bit) {
    // code b100: val 0x03, b111 direct, code b101 : val 0x4, b000 0000 padding
    std::vector<uint8_t> encoded = {0x9E, 0x80};
    std::istrstream is(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    decoder.setData(&is);
    auto byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x03);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0x07);
    
    byte = decoder.nextByte();
    EXPECT_EQ(byte, 0x04);
}

}
