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
    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x00);
}

TEST_F(HuffmanDecoderTest, Decode3bit) {
    std::vector<uint8_t> encoded = {0x80};
    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x03);
}

TEST_F(HuffmanDecoderTest, Decode4bit) {
    std::vector<uint8_t> encoded = {0xE0};

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x06);
}

TEST_F(HuffmanDecoderTest, Decode5bit) {
    std::vector<uint8_t> encoded = {0xF0};

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x07);
}

TEST_F(HuffmanDecoderTest, Decode6bit) {
    std::vector<uint8_t> encoded = {0xF8};

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x08);
}

TEST_F(HuffmanDecoderTest, Decode7bit) {
    std::vector<uint8_t> encoded = {0xFC};

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x09);
}

TEST_F(HuffmanDecoderTest, Decode8bit) {
    std::vector<uint8_t> encoded = {0xFE};

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x0a);
}

TEST_F(HuffmanDecoderTest, Decode9bit) {
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x00};

    decoder.setData(encoded);
    auto byte = decoder.nextXBits(9);
    EXPECT_EQ(byte, 0x1FE);
}

TEST_F(HuffmanDecoderTest, HuffmanDecode9bit) {
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x00};

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x0b);
}

TEST_F(HuffmanDecoderTest, Decode3bitThen3bit) {
    // code b100: val 0x03, code b101 : val 0x4, b00 padding
    std::vector<uint8_t> encoded = {0x94};

    decoder.setData(encoded);
    auto byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0b100);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0b101);
}

TEST_F(HuffmanDecoderTest, HuffmanDecode3bitThen3bit) {
    // code b100: val 0x03, code b101 : val 0x4, b00 padding
    std::vector<uint8_t> encoded = {0x94};

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x03);
    
    byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x04);
}

TEST_F(HuffmanDecoderTest, Decode8bitThen3bit) {
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x00};

    decoder.setData(encoded);
    auto byte = decoder.nextXBits(8);
    EXPECT_EQ(byte, 0xFF);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0x00);
}

TEST_F(HuffmanDecoderTest, Decode3bitThen5bitThen3bit) {
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x00};

    decoder.setData(encoded);
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

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x03);
    
    byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x0b);
}

TEST_F(HuffmanDecoderTest, Decode9bitThen8bitThen3bit) {
    // code b1 1111 1110 : val 0xb, code b100: val 0x03, b0000 padding
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x40};

    decoder.setData(encoded);
    auto byte = decoder.nextXBits(9);
    EXPECT_EQ(byte, 0x1fe);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0x04);
}

TEST_F(HuffmanDecoderTest, HuffmanDecode9bitThen3bit) {
    // code b1 1111 1110 : val 0xb, code b100: val 0x03, b0000 padding
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x40};

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x0b);
    
    byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x03);
}

TEST_F(HuffmanDecoderTest, Decode9bitThen9bit) {
    // code b1 1111 1110 : val 0xb, code b1 1111 1110 : val 0xb, b00 0000 1111 1111 padding
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x7F, 0x80};

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x0b);
    
    byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x0b);
}

TEST_F(HuffmanDecoderTest, Decode6bitThen9bit) {
    // code b111110: val 0x08, code b1 1111 1110 : val 0xb, b00 0000 0000 padding
    std::vector<uint8_t> encoded = {0xFB, 0xFC, 0x00};

    decoder.setData(encoded);
    auto byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x08);
    
    byte = decoder.nextHuffmanByte();
    EXPECT_EQ(byte, 0x0b);
}

TEST_F(HuffmanDecoderTest, DecodeMarkerSegmentException) {
    std::vector<uint8_t> encoded = {0xFF, 0xD0};

    decoder.setData(encoded);
    EXPECT_THROW(decoder.nextHuffmanByte(), ResetMarkerException);
}

TEST_F(HuffmanDecoderTest, PeekAndReadMarkerSegment) {
    std::vector<uint8_t> encoded = {0xFF, 0xD0};

    decoder.setData(encoded);
    EXPECT_EQ(decoder.peakXBits(16), 0x0000);
    EXPECT_THROW(decoder.nextXBits(16), ResetMarkerException);
}

TEST_F(HuffmanDecoderTest, PeekAndReadAheadofMarkerSegment) {
    std::vector<uint8_t> encoded = {0xD0, 0xFF, 0xD0, 0x00};

    decoder.setData(encoded);
    EXPECT_EQ(decoder.peakXBits(4), 0xD);
    EXPECT_EQ(decoder.peakXBits(16), 0xD000);
    EXPECT_EQ(decoder.nextXBits(4), 0x0D);
    EXPECT_EQ(decoder.nextXBits(4), 0x00);
    EXPECT_THROW(decoder.nextXBits(16), ResetMarkerException);
}

TEST_F(HuffmanDecoderTest, PeekAndReadAheadofMarkerSegment2) {
    std::vector<uint8_t> encoded = {0xD0, 0xFF, 0xD0, 0x00};

    decoder.setData(encoded);
    EXPECT_EQ(decoder.peakXBits(16), 0xD000);
    EXPECT_EQ(decoder.nextXBits(4), 0x0D);
    EXPECT_EQ(decoder.nextXBits(4), 0x00);
    EXPECT_THROW(decoder.nextXBits(16), ResetMarkerException);
}

TEST_F(HuffmanDecoderTest, NotEnoughBytesErrorDueToNoBytes) {
    std::vector<uint8_t> encoded = {0xFF};

    decoder.setData(encoded);
    
    EXPECT_THROW(decoder.nextXBits(9), std::runtime_error);
}

TEST_F(HuffmanDecoderTest, NotEnoughBytesErrorDueToMarkerSegment) {
    std::vector<uint8_t> encoded = {0xFF, 0x00};

    decoder.setData(encoded);
    
    EXPECT_THROW(decoder.nextXBits(9), std::runtime_error);
}

TEST_F(HuffmanDecoderTest, Decode3bitSkip3Then3bit) {
    // code b100: val 0x03, b111 direct, code b101 : val 0x4, b000 0000 padding
    std::vector<uint8_t> encoded = {0x9E, 0x80};

    decoder.setData(encoded);
    auto huffmanByte = decoder.nextHuffmanByte();
    EXPECT_EQ(huffmanByte, 0x03);
    
    auto byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0x07);
    
    huffmanByte = decoder.nextHuffmanByte();
    EXPECT_EQ(huffmanByte, 0x04);
}

TEST_F(HuffmanDecoderTest, PeakAndRead3bitThen3bit) {
    // code b100: val 0x03, code b101 : val 0x4, b00 padding
    std::vector<uint8_t> encoded = {0x94};

    decoder.setData(encoded);
    
    auto byte = decoder.peakXBits(3);
    EXPECT_EQ(byte, 0b100);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0b100);
    
    byte = decoder.peakXBits(3);
    EXPECT_EQ(byte, 0b101);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0b101);
}

TEST_F(HuffmanDecoderTest, PeakAndRead8bitThen3bit) {
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x00};

    decoder.setData(encoded);
    auto byte = decoder.peakXBits(8);
    EXPECT_EQ(byte, 0xFF);
    
    byte = decoder.nextXBits(8);
    EXPECT_EQ(byte, 0xFF);
    
    byte = decoder.peakXBits(3);
    EXPECT_EQ(byte, 0x00);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0x00);
}

TEST_F(HuffmanDecoderTest, PeakAndRead3bitThen5bitThen3bit) {
    std::vector<uint8_t> encoded = {0xFF, 0x00, 0x00};

    decoder.setData(encoded);
    auto byte = decoder.peakXBits(3);
    EXPECT_EQ(byte, 0b111);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0b111);
    
    byte = decoder.peakXBits(5);
    EXPECT_EQ(byte, 0b11111);
    
    byte = decoder.nextXBits(5);
    EXPECT_EQ(byte, 0b11111);
    
    byte = decoder.peakXBits(3);
    EXPECT_EQ(byte, 0x00);
    
    byte = decoder.nextXBits(3);
    EXPECT_EQ(byte, 0x00);
}

TEST_F(HuffmanDecoderTest, Peak9bitFailRead) {
    std::vector<uint8_t> encoded = {0xF0};

    decoder.setData(encoded);
    auto byte = decoder.peakXBits(9);
    EXPECT_EQ(byte, 0x1e0);
    
    EXPECT_THROW(decoder.nextXBits(9), std::runtime_error);
}

TEST_F(HuffmanDecoderTest, Peak16bitFailRead) {
    std::vector<uint8_t> encoded = {0xF0};

    decoder.setData(encoded);
    auto byte = decoder.peakXBits(16);
    EXPECT_EQ(byte, 0xF000);
    
    EXPECT_THROW(decoder.nextXBits(16), std::runtime_error);
}

TEST_F(HuffmanDecoderTest, Peak16bitRead4bit) {
    std::vector<uint8_t> encoded = {0xF0};

    decoder.setData(encoded);
    auto byte = decoder.peakXBits(16);
    EXPECT_EQ(byte, 0xF000);
    
    byte = decoder.nextXBits(4);
    EXPECT_EQ(byte, 0x0F);
    
    byte = decoder.peakXBits(16);
    EXPECT_EQ(byte, 0x0000);
    
    byte = decoder.nextXBits(4);
    EXPECT_EQ(byte, 0x00);
    
    EXPECT_THROW(decoder.nextXBits(8), std::runtime_error);
}

TEST_F(HuffmanDecoderTest, Peak16bitRead4and4) {
    std::vector<uint8_t> encoded = {0xF5, 0xAD};

    decoder.setData(encoded);
    auto byte = decoder.peakXBits(16);
    EXPECT_EQ(byte, 0xF5AD);
    
    byte = decoder.nextXBits(4);
    EXPECT_EQ(byte, 0x0F);
    
    byte = decoder.peakXBits(16);
    EXPECT_EQ(byte, 0x5AD0);
    
    byte = decoder.nextXBits(4);
    EXPECT_EQ(byte, 0x05);
    
    byte = decoder.nextXBits(4);
    EXPECT_EQ(byte, 0x0A);
    
    byte = decoder.nextXBits(4);
    EXPECT_EQ(byte, 0x0D);
}

}
