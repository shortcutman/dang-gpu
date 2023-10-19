//
//  idct_test.cpp
//  danpg-tests
//
//  Created by Daniel Burke on 1/8/2023.
//

#include <gtest/gtest.h>
#include <string>

#include "idct.hpp"

namespace {

TEST(IDCTTest, InputAndOutputOfLumaFromTestImageFloat) {
    image::DataUnit input = {
        -430, -10, 20, 0, 0, 0, 0, 0,
        20, 0, 0, 0, 0, 0, 0, 0,
        -20, 10, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    image::DataUnit output = {
        -49, -51, -54, -56, -56, -55, -52, -50,
        -49, -51, -53, -55, -55, -53, -50, -48,
        -49, -51, -53, -54, -53, -50, -46, -44,
        -50, -51, -53, -53, -52, -48, -45, -42,
        -51, -53, -54, -55, -53, -50, -46, -43,
        -53, -55, -57, -58, -57, -54, -50, -48,
        -55, -57, -59, -61, -61, -58, -56, -53,
        -56, -58, -61, -63, -63, -62, -59, -57
    };
    
    auto result = image::idct_float(input);
    EXPECT_EQ(result, output);
}

TEST(IDCTTest, InputAndOutputOfLumaFromTestImageFloatTable) {
    image::DataUnit input = {
        -430, -10, 20, 0, 0, 0, 0, 0,
        20, 0, 0, 0, 0, 0, 0, 0,
        -20, 10, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    image::DataUnit output = {
        -49, -51, -54, -56, -56, -55, -52, -50,
        -49, -51, -53, -55, -55, -53, -50, -48,
        -49, -51, -53, -54, -53, -50, -46, -44,
        -50, -51, -53, -53, -52, -48, -45, -42,
        -51, -53, -54, -55, -53, -50, -46, -43,
        -53, -55, -57, -58, -57, -54, -50, -48,
        -55, -57, -59, -61, -61, -58, -56, -53,
        -56, -58, -61, -63, -63, -62, -59, -57
    };
    
    auto result = image::idct_float_table(input);
    EXPECT_EQ(result, output);
}

//TEST(IDCTTest, DCTFloatLoeffler) {
//    image::DataUnit input = {
//        -430, -10, 20, 0, 0, 0, 0, 0,
//        20, 0, 0, 0, 0, 0, 0, 0,
//        -20, 10, 0, 0, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 0
//    };
//    image::DataUnit output = {
//        -49, -51, -54, -56, -56, -55, -52, -50,
//        -49, -51, -53, -55, -55, -53, -50, -48,
//        -49, -51, -53, -54, -53, -50, -46, -44,
//        -50, -51, -53, -53, -52, -48, -45, -42,
//        -51, -53, -54, -55, -53, -50, -46, -43,
//        -53, -55, -57, -58, -57, -54, -50, -48,
//        -55, -57, -59, -61, -61, -58, -56, -53,
//        -56, -58, -61, -63, -63, -62, -59, -57
//    };
//    
//    auto result = image::idct_float_loeffler(output);
//    EXPECT_EQ(result, input);
//}

TEST(IDCTTest, IDCTFloatLoeffler) {
    image::DataUnit input = {
        -430, -10, 20, 0, 0, 0, 0, 0,
        20, 0, 0, 0, 0, 0, 0, 0,
        -20, 10, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    image::DataUnit output = {
        -49, -51, -54, -56, -56, -55, -52, -50,
        -49, -51, -53, -55, -55, -53, -50, -48,
        -49, -51, -53, -54, -53, -50, -46, -44,
        -50, -51, -53, -53, -52, -48, -45, -42,
        -51, -53, -54, -55, -53, -50, -46, -43,
        -53, -55, -57, -58, -57, -54, -50, -48,
        -55, -57, -59, -61, -61, -58, -56, -53,
        -56, -58, -61, -63, -63, -62, -59, -57
    };

    image::idct_float_loeffler(input);
    EXPECT_EQ(input, output);
}

TEST(IDCTTest, InputAndOutputOfLumaFromTestImageInteger) {
    image::DataUnit input = {
        -430, -10, 20, 0, 0, 0, 0, 0,
        20, 0, 0, 0, 0, 0, 0, 0,
        -20, 10, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    image::DataUnit output = {
        -48, -51, -53, -56, -56, -54, -52, -50,
        -48, -50, -53, -55, -54, -52, -49, -47,
        -48, -50, -52, -53, -52, -49, -46, -43,
        -49, -50, -52, -53, -51, -48, -44, -41,
        -50, -52, -53, -54, -52, -49, -45, -42,
        -52, -54, -56, -57, -56, -53, -49, -47,
        -54, -56, -58, -60, -60, -58, -55, -53,
        -55, -57, -60, -63, -63, -61, -58, -57
    };
    
    auto result = image::idct_int(input);
    
    EXPECT_EQ(result, output);
}

TEST(IDCTTest, InputAndOutputOfLumaFromTestImageIntegerTable) {
    image::DataUnit input = {
        -430, -10, 20, 0, 0, 0, 0, 0,
        20, 0, 0, 0, 0, 0, 0, 0,
        -20, 10, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    image::DataUnit output = {
        -48, -51, -53, -56, -56, -54, -52, -50,
        -48, -50, -53, -55, -54, -52, -49, -47,
        -48, -50, -52, -53, -52, -49, -46, -43,
        -49, -50, -52, -53, -51, -48, -44, -41,
        -50, -52, -53, -54, -52, -49, -45, -42,
        -52, -54, -56, -57, -56, -53, -49, -47,
        -54, -56, -58, -60, -60, -58, -55, -53,
        -55, -57, -60, -63, -63, -61, -58, -57
    };
    
    auto result = image::idct_int_table(input);
    
    EXPECT_EQ(result, output);
}

//TEST(Loeffler, SymmetryTestZeroes) {
//    std::array<int, 8> in = {0, 0, 0, 0, 0, 0, 0, 0};
//
//    auto r1 = image::loeffler_1d_dct(in);
//    auto r2 = image::loeffler_1d_idct(r1);
//
//    EXPECT_EQ(in, r2);
//}
//
//TEST(Loeffler, SymmetryTestOnes) {
//    std::array<int, 8> in = {1, 1, 1, 1, 1, 1, 1, 1};
//
//    auto r1 = image::loeffler_1d_dct(in);
//    auto r2 = image::loeffler_1d_idct(r1);
//
//    EXPECT_EQ(in, r2);
//}
//
//TEST(Loeffler, SymmetryTestNegOnes) {
//    std::array<int, 8> in = {-1, -1, -1, -1, -1, -1, -1, -1};
//
//    auto r1 = image::loeffler_1d_dct(in);
//    auto r2 = image::loeffler_1d_idct(r1);
//
//    EXPECT_EQ(in, r2);
//}
//
//TEST(Loeffler, ComparisonTest) {
//    std::array<int, 8> in = {3, 2, 1, 0, -1, -2, -3, -4};
//
//    auto mine = image::loeffler_1d_idct(in);
//    auto res = image::loeffler_1d_dct(mine);
//
//    EXPECT_EQ(in, res);
//}

}
