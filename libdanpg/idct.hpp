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

DataUnit idct_float(const DataUnit& du);
DataUnit idct_float_table(const DataUnit& du);
DataUnit dct_float_loeffler(const DataUnit& du);
DataUnit idct_float_loeffler(const DataUnit& du);
DataUnit idct_int(const DataUnit& du);
DataUnit idct_int_table(const DataUnit& du);

#define idct idct_float_loeffler

std::array<int, 8> loeffler_1d_dct(const std::array<int, 8> in);

inline std::array<float, 8> loeffler_1d_idct_col(std::array<float, 8*8>& in, int offset);
inline std::array<float, 8> loeffler_1d_idct_row(const int* const in);

}

#endif /* idct_hpp */
