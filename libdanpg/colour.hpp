//
//  colour.hpp
//  libdanpg
//
//  Created by Daniel Burke on 2/8/2023.
//

#ifndef colour_hpp
#define colour_hpp

#include <tuple>

namespace image {

typedef std::tuple<int, int, int> Colour;

Colour ycbcrToRGB(const Colour& ycbcr);

}

#endif /* colour_hpp */
