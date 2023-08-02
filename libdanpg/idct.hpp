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

DataUnit idct(const DataUnit& du);

}

#endif /* idct_hpp */
