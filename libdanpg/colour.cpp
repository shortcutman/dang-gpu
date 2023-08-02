//
//  colour.cpp
//  libdanpg
//
//  Created by Daniel Burke on 2/8/2023.
//

#include "colour.hpp"

using namespace image;

namespace {

int adjustAndClamp(float val) {
    val += 128.f;
    
    if (val > 255) {
        return 255;
    } else if (val < 0) {
        return 0;
    } else {
        return static_cast<int>(val);
    }
}

}

Colour image::ycbcrToRGB(const Colour& ycbcr) {
    float y = std::get<0>(ycbcr);
    float cb = std::get<1>(ycbcr);
    float cr = std::get<2>(ycbcr);
    auto r = adjustAndClamp(y + (1.402f * cr));
    auto g = adjustAndClamp(y - (0.34414f * cb) - (0.71414f * cr));
    auto b = adjustAndClamp(y + (1.772f * cb));
    return std::make_tuple(r, g, b);
}

void image::ycbcrToRGBInPlace(std::span<Colour> data) {
    for (auto& c : data) {
        c = ycbcrToRGB(c);
    }
}
