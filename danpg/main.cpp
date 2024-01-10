//
//  main.cpp
//  danpg
//
//  Created by Daniel Burke on 2/5/2023.
//

#include <iostream>
#include <fstream>

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

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
        
        NS::SharedPtr<NS::AutoreleasePool> _pool;
        NS::SharedPtr<MTL::Device> _metalDevice;
        _pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
        _metalDevice = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
        
        runFuncTimed([&]() {
            f.ignore(std::numeric_limits<std::streamsize>::max());
            std::streamsize length = f.gcount();
            f.clear();
            f.seekg(0, std::ios_base::beg);
            
            std::vector<uint8_t> data;
            data.resize(length);
            f.read(reinterpret_cast<char*>(&data[0]), length);
            
            jpeg = image::Jpeg(data, _metalDevice.get());
        });
        
        image::writeOutPPM("/private/tmp/jpeg.ppm", jpeg._x, jpeg._y, std::span{jpeg._image, jpeg._x * jpeg._y * sizeof(image::Colour)});
    }
    
    return 0;
}
