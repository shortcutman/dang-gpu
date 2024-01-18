//
//  duCopy.metal
//  libdanpg
//
//  Created by Daniel Burke on 10/1/2024.
//

#include <metal_stdlib>
using namespace metal;

kernel void copyLumaToImage(device const int* imgCompData, device int4* imageData, uint2 location [[thread_position_in_grid]], uint2 gridSize [[threads_per_grid]]) {
    
    imageData[location.y * gridSize.x + location.x].x = imgCompData[location.y * gridSize.x + location.x];
}

kernel void copyChromaBlue(device const int* imgCompData, device int4* imageData, uint2 location [[thread_position_in_grid]], uint2 gridSize [[threads_per_grid]]) {
    
    imageData[location.y * gridSize.x + location.x].y = imgCompData[(location.y / 2) * (gridSize.x / 2) + (location.x / 2)];
}

kernel void copyChromaRed(device const int* imgCompData, device int4* imageData, uint2 location [[thread_position_in_grid]], uint2 gridSize [[threads_per_grid]]) {
    
    imageData[location.y * gridSize.x + location.x].z = imgCompData[(location.y / 2) * (gridSize.x / 2) + (location.x / 2)];
}
