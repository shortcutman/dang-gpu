//
//  idct.hpp
//  libdanpg
//
//  Created by Daniel Burke on 2/8/2023.
//

#ifndef idct_hpp
#define idct_hpp

#include <array>

namespace image {

typedef std::array<int, 8*8> DataUnit;

DataUnit idct_original(const DataUnit& du);

#define idct idct_original

}

#endif /* idct_hpp */
