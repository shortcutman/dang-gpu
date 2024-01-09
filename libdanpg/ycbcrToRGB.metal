//
//  ycbcrToRGB.metal
//  libdanpg
//
//  Created by Daniel Burke on 9/1/2024.
//

//#include <metal_stdlib>
//using namespace metal;
//
//inline int adjustAndClamp(float val) {
//    val += 128.f;
//    
//    if (val > 255) {
//        return 255;
//    } else if (val < 0) {
//        return 0;
//    } else {
//        return static_cast<int>(val);
//    }
//}
//
//}
//
//Colour image::ycbcrToRGB(const Colour& ycbcr) {
//    auto r = adjustAndClamp(ycbcr.y + (1.402f * ycbcr.cr));
//    auto g = adjustAndClamp(ycbcr.y - (0.34414f * ycbcr.cb) - (0.71414f * ycbcr.cr));
//    auto b = adjustAndClamp(ycbcr.y + (1.772f * ycbcr.cb));
//    return {r, g, b};
//}
//
//void image::ycbcrToRGBOverMCU(device int* data,
//                              int xWidth,
//                              uint xIndex,
//                              uint yIndex) {
//    data[yIndex * xWidth + xIndex]
//    for (size_t y = yStart; y < (yStart + 16); y++) {
//        for (size_t x = xStart; x < (xStart + 16); x++) {
//            data[y * width + x] = ycbcrToRGB(data[y * width + x]);
//        }
//    }
//}
