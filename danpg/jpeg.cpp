//
//  jpeg.cpp
//  danpg
//
//  Created by Daniel Burke on 2/5/2023.
//

#include "jpeg.hpp"

#include <cstddef>
#include <iostream>

Jpeg::Jpeg(std::istream& is) {
    
    uint8_t magic = 0x00;
    is.read(reinterpret_cast<char*>(&magic), 1);
    
    if (magic != 0xFF) {
        std::cout << "not jpeg" << std::endl;
        return;
    } else {
        std::cout << "jpeg" << std::endl;
    }
}
