//
//  main.cpp
//  danpg
//
//  Created by Daniel Burke on 2/5/2023.
//

#include <iostream>
#include <fstream>

#include "jpeg.hpp"
#include "colour.hpp"

void runFuncTimed(std::function<void(void)> func) {
    std::cout << "Jpeg decode start" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    func();
    
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Jpeg decode duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
}

int main(int argc, const char * argv[]) {
    
    std::fstream f("/Users/daniel/Projects.nosync/danpg/danpg/danpg/image2.jpg");
    
    if (f.bad()) {
        std::cout << "unable to open" << std::endl;
    } else {
        image::Jpeg jpeg;
        
        std::cout << "opened" << std::endl;
        
        runFuncTimed([&]() {
            jpeg = image::Jpeg(f);
        });
        
        image::writeOutPPM("/private/tmp/jpeg.ppm", jpeg._x, jpeg._y, std::span{jpeg._image, jpeg._x * jpeg._y * sizeof(image::Colour)});
    }
    
    return 0;
}
