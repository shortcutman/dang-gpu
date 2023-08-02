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
    Colour ycbcr = std::make_tuple(-49, -14, 14);
    auto conversion = ycbcrToRGB(ycbcr);
    EXPECT_EQ(conversion, std::make_tuple(98, 73, 54));
}

}
