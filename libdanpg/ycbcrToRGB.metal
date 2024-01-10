//
//  ycbcrToRGB.metal
//  libdanpg
//
//  Created by Daniel Burke on 9/1/2024.
//

#include <metal_stdlib>
using namespace metal;

int adjustAndClamp(float val) {
    val += 128.f;
    
    if (val > 255) {
        return 255;
    } else if (val < 0) {
        return 0;
    } else {
        return static_cast<int>(val);
    }
}

kernel void ycbcrToRGB(device int3* data, uint2 location [[thread_position_in_grid]], uint2 gridSize [[threads_per_grid]]) {
    int3 pixelIn = data[location.y * gridSize.x + location.x];
    int3 pixelOut;
    
    pixelOut.x = adjustAndClamp(pixelIn.x + (1.402f * pixelIn.z));
    pixelOut.y = adjustAndClamp(pixelIn.x - (0.34414f * pixelIn.y) - (0.71414f * pixelIn.z));
    pixelOut.z = adjustAndClamp(pixelIn.x + (1.772f * pixelIn.y));
    
    data[location.y * gridSize.x + location.x] = pixelOut;
}
