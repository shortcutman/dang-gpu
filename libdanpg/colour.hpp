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

//typedef std::array<int, 3> Colour;

struct Colour {
    union {
        int r;
        int y;
    };
    
    union {
        int g;
        int cr;
    };
    
    union {
        int b;
        int cb;
    };
};

Colour ycbcrToRGB(const Colour& ycbcr);
void ycbcrToRGBInPlace(std::span<Colour> data);

void writeOutPPM(std::string filepath, size_t width, size_t height, std::span<Colour> data);

}

#endif /* colour_hpp */
