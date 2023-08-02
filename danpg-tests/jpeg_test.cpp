//
//  jpeg_test.cpp
//  danpg-tests
//
//  Created by Daniel Burke on 1/8/2023.
//

#include <gtest/gtest.h>
#include <string>

#include "jpeg.hpp"

namespace {

TEST(IDCTTest, InputAndOutputOfLumaFromTestImage) {
    image::Jpeg::DataUnit input = {
        -430, -10, 20, 0, 0, 0, 0, 0,
        20, 0, 0, 0, 0, 0, 0, 0,
        -20, 10, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    image::Jpeg::DataUnit output = {
        -49, -51, -54, -56, -56, -55, -52, -50,
        -49, -51, -53, -55, -55, -53, -50, -48,
        -49, -51, -53, -54, -53, -50, -46, -44,
        -50, -51, -53, -53, -52, -48, -45, -42,
        -51, -53, -54, -55, -53, -50, -46, -43,
        -53, -55, -57, -58, -57, -54, -50, -48,
        -55, -57, -59, -61, -61, -58, -56, -53,
        -56, -58, -61, -63, -63, -62, -59, -57
    };
    
    image::Jpeg j;
    auto result = j.idct(input);
    EXPECT_EQ(result, output);
}

}
