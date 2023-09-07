//
//  idct.cpp
//  libdanpg
//
//  Created by Daniel Burke on 2/8/2023.
//

#include "idct.hpp"

#include <cmath>
#include <numbers>

image::DataUnit image::idct_float(const DataUnit& du) {
    DataUnit out;
    
    for (size_t y = 0; y < 8; y++) {
        for (size_t x = 0; x < 8; x++) {
            float val = 0.f;
            
            for (size_t u = 0; u < 8; u++) {
                for (size_t v = 0; v < 8; v++) {
                    float cu = (u == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    float cv = (v == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    val += cu * cv * du[u + v*8]
                            * std::cos((2.f * x + 1.f) * u * std::numbers::pi / 16.f)
                            * std::cos((2.f * y + 1.f) * v * std::numbers::pi / 16.f);
                }
            }
            val *= 0.25f;
            
            out[x + y * 8] = static_cast<int>(val);
        }
    }
    
    return out;
}

image::DataUnit image::idct_int(const DataUnit& du) {
    DataUnit out;
    
    for (size_t y = 0; y < 8; y++) {
        for (size_t x = 0; x < 8; x++) {
            int val = 0.f;
            
            for (size_t u = 0; u < 8; u++) {
                for (size_t v = 0; v < 8; v++) {
                    float cu = (u == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    float cv = (v == 0) ? (1.f/std::sqrt(2.f)) : 1.f;
                    val += cu * cv * du[u + v*8]
                            * std::cos((2.f * x + 1.f) * u * std::numbers::pi / 16.f)
                            * std::cos((2.f * y + 1.f) * v * std::numbers::pi / 16.f);
                }
            }
            val *= 0.25f;
            
            out[x + y * 8] = static_cast<int>(val);
        }
    }
    
    return out;
}
