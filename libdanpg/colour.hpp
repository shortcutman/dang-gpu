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
    
    inline void setIndexColour(size_t index, int value) {
        switch (index) {
            case 0:
                y = value;
                break;
            case 1:
                cb = value;
                break;
            case 2:
                cr = value;
                break;
            default:
                throw std::logic_error("index out of bounds for colour");
                break;
        }
    }
};

Colour ycbcrToRGB(const Colour& ycbcr);
void ycbcrToRGBInPlace(std::span<Colour> data);

void writeOutPPM(std::string filepath, size_t width, size_t height, std::span<Colour> data);

}

#endif /* colour_hpp */
