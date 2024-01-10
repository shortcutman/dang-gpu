//
//  colour.cpp
//  libdanpg
//
//  Created by Daniel Burke on 2/8/2023.
//

#include "colour.hpp"

#include <fstream>

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

using namespace image;

namespace {

inline int adjustAndClamp(float val) {
    val += 128.f;
    
    if (val > 255) {
        return 255;
    } else if (val < 0) {
        return 0;
    } else {
        return static_cast<int>(val);
    }
}

}

Colour image::ycbcrToRGB(const Colour& ycbcr) {
    auto r = adjustAndClamp(ycbcr.y + (1.402f * ycbcr.cr));
    auto g = adjustAndClamp(ycbcr.y - (0.34414f * ycbcr.cb) - (0.71414f * ycbcr.cr));
    auto b = adjustAndClamp(ycbcr.y + (1.772f * ycbcr.cb));
    return {r, g, b};
}

void image::ycbcrToRGBOverMCU(Colour *data, size_t width, size_t xStart, size_t yStart) {
    for (size_t y = yStart; y < (yStart + 16); y++) {
        for (size_t x = xStart; x < (xStart + 16); x++) {
            data[y * width + x] = ycbcrToRGB(data[y * width + x]);
        }
    }
}

void image::ycbcrToRGB_accel(Colour *data, size_t width, size_t height) {
    NS::SharedPtr<NS::AutoreleasePool> _pool;
    NS::SharedPtr<MTL::Device> _metalDevice;
    NS::SharedPtr<MTL::CommandQueue> _commandQueue;
    _pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
    _metalDevice = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    _commandQueue = NS::TransferPtr(_metalDevice->newCommandQueue());
    auto defaultLib = _metalDevice->newDefaultLibrary();
    auto function = defaultLib->newFunction(MTLSTR("ycbcrToRGB"));
    NS::Error* error = nullptr;
    auto functionPSO = NS::TransferPtr(_metalDevice->newComputePipelineState(function, &error));
    
    auto buffer = NS::TransferPtr(_metalDevice->newBuffer(data, width * height * sizeof(Colour), MTL::ResourceStorageModeShared, nullptr));

    auto commandBuffer = _commandQueue->commandBuffer();
    auto computeEncoder = commandBuffer->computeCommandEncoder();
    computeEncoder->setComputePipelineState(functionPSO.get());
    computeEncoder->setBuffer(buffer.get(), 0, 0);

    auto gridSize = MTL::Size(width, height, 1);
    auto threadGroupSizeObj = MTL::Size(16, 16, 1);
    
    computeEncoder->dispatchThreads(gridSize, threadGroupSizeObj);
    computeEncoder->endEncoding();
    
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
}

void image::writeOutPPM(std::string filepath, size_t width, size_t height, std::span<Colour> data) {
    std::ofstream file;
    file.open(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing.");
    }
    
    if (width * height > data.size()) {
        throw std::runtime_error("Width and height greater than provided data.");
    }
    
    file << "P3" << std::endl;
    file << width << " " << height << std::endl;
    file << "255" << std::endl;
     
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            auto pixel = data[x + y * width];
            file << std::to_string(pixel.r) << " "
                 << std::to_string(pixel.g) << " "
                 << std::to_string(pixel.b) << " ";
        }
         
        file << std::endl;
    }
    
    file.close();
}
