//
//  colour_test.cpp
//  danpg-tests
//
//  Created by Daniel Burke on 2/8/2023.
//

#include <gtest/gtest.h>
#include <string>

#include "colour.hpp"

using namespace image;

namespace {

TEST(ColourTest, YCbCrToRGB) {
    Colour ycbcr = {-49, -14, 14};
    auto conversion = ycbcrToRGB(ycbcr);
    EXPECT_EQ(conversion, Colour({98, 73, 54}));
}

TEST(ColourTest, IndexTest) {
    Colour ycbcr;
    ycbcr.setIndexColour(0, 11);
    ycbcr.setIndexColour(1, 22);
    ycbcr.setIndexColour(2, 33);
    
    EXPECT_EQ(ycbcr.r, 11);
    EXPECT_EQ(ycbcr.g, 22);
    EXPECT_EQ(ycbcr.b, 33);
    
    EXPECT_EQ(ycbcr.y, 11);
    EXPECT_EQ(ycbcr.cb, 22);
    EXPECT_EQ(ycbcr.cr, 33);
}

}
