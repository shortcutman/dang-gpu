//
//  colour.hpp
//  libdanpg
//
//  Created by Daniel Burke on 2/8/2023.
//

#ifndef colour_hpp
#define colour_hpp

#include <span>
#include <tuple>

namespace image {

typedef std::tuple<int, int, int> Colour;

Colour ycbcrToRGB(const Colour& ycbcr);
void ycbcrToRGBInPlace(std::span<Colour> data);

void writeOutPPM(std::string filepath, size_t width, size_t height, std::span<Colour> data);

}

#endif /* colour_hpp */
