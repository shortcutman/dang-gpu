//
//  idct.metal
//  libdanpg
//
//  Created by Daniel Burke on 21/1/2024.
//

#include <metal_stdlib>
using namespace metal;

void copyImgCmpDataToIntermediate(device const int* in, thread int* out, int span);
void copyIntermediateToImgCmpData(thread const int* in, device int* out, int span);
void loeffler_1d_idct_row(thread const int* in, thread float* inter, int offset);
void loeffler_1d_idct_col(thread float* in, thread int* out, int offset);

kernel void idct(device int* imgCompData, uint2 location [[thread_position_in_grid]], uint2 gridSize [[threads_per_grid]]) {
    int du[8*8];
    float intermediate[8*8];
    
    int imgLocX = location.x * 8;
    int imgLocY = location.y * 8;
    int imgWidth = gridSize.x * 8;
    
    int startPosition = imgLocX + imgLocY * imgWidth;
    
    copyImgCmpDataToIntermediate(imgCompData + startPosition, du, imgWidth);
    
    //IDCT Row
    for (int y = 0; y < 8; y++) {
        loeffler_1d_idct_row(du, intermediate, y * 8);
    }
    
    //IDCT Column
    for (int x = 0; x < 8; x++) {
        loeffler_1d_idct_col(intermediate, du, x);
    }
    
    copyIntermediateToImgCmpData(du, imgCompData + startPosition, imgWidth);
}

void copyImgCmpDataToIntermediate(device const int* imgCompData, thread int* du, int width) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            du[x + y * 8] = imgCompData[x + y * width];
        }
    }
}

void copyIntermediateToImgCmpData(thread const int* du, device int* imgCompData, int width) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            imgCompData[x + y * width] = du[x + y * 8];
        }
    }
}

void loeffler_1d_idct_row(thread const int* in, thread float* inter, int offset) {
    ///
    //stage4
    ///
    float stage4[8];
    stage4[0] = in[0 + offset];
    stage4[1] = in[4 + offset];
    stage4[2] = in[2 + offset];
    stage4[3] = in[6 + offset];
    stage4[4] = in[1 + offset] - in[7 + offset];
    stage4[5] = in[3 + offset] * sqrt(2.f);
    stage4[6] = in[5 + offset] * sqrt(2.f);
    stage4[7] = in[1 + offset] + in[7 + offset];
    
    ///
    //stage3
    ///
    float stage3[8];
    stage3[0] = stage4[0] + stage4[1];
    stage3[1] = stage4[0] - stage4[1];
    stage3[2] = stage4[2] * sqrt(2.f) * cos(6.f * M_PI_F / 16.f)
                - stage4[3] * sqrt(2.f) * sin(6.f * M_PI_F / 16.f );
    stage3[3] = stage4[2] * sqrt(2.f) * sin(6.f * M_PI_F / 16.f)
                + stage4[3] * sqrt(2.f) * cos(6.f * M_PI_F / 16.f );
    stage3[4] = stage4[4] + stage4[6];
    stage3[5] = stage4[7] - stage4[5];
    stage3[6] = stage4[4] - stage4[6];
    stage3[7] = stage4[7] + stage4[5];
    
    ///
    //stage 2
    ///
    float stage2[8];
    stage2[0] = stage3[0] + stage3[3];
    stage2[1] = stage3[1] + stage3[2];
    stage2[2] = stage3[1] - stage3[2];
    stage2[3] = stage3[0] - stage3[3];
    //stage 2, c3
    stage2[4] = stage3[4] * cos(3.f * M_PI_F / 16.f)
                - stage3[7] * sin(3.f * M_PI_F / 16.f );
    stage2[7] =  stage3[4] * sin(3.f * M_PI_F / 16.f)
                + stage3[7] * cos(3.f * M_PI_F / 16.f );
    //stage 2, c1
    stage2[5] = stage3[5] * cos(1.f * M_PI_F / 16.f)
                - stage3[6] * sin(1.f * M_PI_F / 16.f );
    stage2[6] =  stage3[5] * sin(1.f * M_PI_F / 16.f)
                + stage3[6] * cos(1.f * M_PI_F / 16.f );
    
    ///
    //stage 1
    ///
    inter[0 + offset] = stage2[0] + stage2[7];
    inter[1 + offset] = stage2[1] + stage2[6];
    inter[2 + offset] = stage2[2] + stage2[5];
    inter[3 + offset] = stage2[3] + stage2[4];
    inter[4 + offset] = stage2[3] - stage2[4];
    inter[5 + offset] = stage2[2] - stage2[5];
    inter[6 + offset] = stage2[1] - stage2[6];
    inter[7 + offset] = stage2[0] - stage2[7];

}

void loeffler_1d_idct_col(thread float* in, thread int* out, int offset) {
    ///
    //stage4
    ///
    float stage4[8];
    stage4[0] = in[0 + offset];
    stage4[1] = in[32 + offset];
    stage4[2] = in[16 + offset];
    stage4[3] = in[48 + offset];
    stage4[4] = in[8 + offset] - in[56 + offset];
    stage4[5] = in[24 + offset] * sqrt(2.f);
    stage4[6] = in[40 + offset] * sqrt(2.f);
    stage4[7] = in[8 + offset] + in[56 + offset];
    
    ///
    //stage3
    ///
    float stage3[8];
    stage3[0] = stage4[0] + stage4[1];
    stage3[1] = stage4[0] - stage4[1];
    stage3[2] = stage4[2] * sqrt(2.f) * cos(6.f * M_PI_F / 16.f)
                - stage4[3] * sqrt(2.f) * sin(6.f * M_PI_F / 16.f );
    stage3[3] = stage4[2] * sqrt(2.f) * sin(6.f * M_PI_F / 16.f)
                + stage4[3] * sqrt(2.f) * cos(6.f * M_PI_F / 16.f );
    stage3[4] = stage4[4] + stage4[6];
    stage3[5] = stage4[7] - stage4[5];
    stage3[6] = stage4[4] - stage4[6];
    stage3[7] = stage4[7] + stage4[5];
    
    ///
    //stage 2
    ///
    float stage2[8];
    stage2[0] = stage3[0] + stage3[3];
    stage2[1] = stage3[1] + stage3[2];
    stage2[2] = stage3[1] - stage3[2];
    stage2[3] = stage3[0] - stage3[3];
    //stage 2, c3
    stage2[4] = stage3[4] * cos(3.f * M_PI_F / 16.f)
                - stage3[7] * sin(3.f * M_PI_F / 16.f );
    stage2[7] =  stage3[4] * sin(3.f * M_PI_F / 16.f)
                + stage3[7] * cos(3.f * M_PI_F / 16.f );
    //stage 2, c1
    stage2[5] = stage3[5] * cos(1.f * M_PI_F / 16.f)
                - stage3[6] * sin(1.f * M_PI_F / 16.f );
    stage2[6] =  stage3[5] * sin(1.f * M_PI_F / 16.f)
                + stage3[6] *cos(1.f * M_PI_F / 16.f );
    
    ///
    //stage 1
    ///
    out[0*8 + offset] = (stage2[0] + stage2[7]) / 8.f;
    out[1*8 + offset] = (stage2[1] + stage2[6]) / 8.f;
    out[2*8 + offset] = (stage2[2] + stage2[5]) / 8.f;
    out[3*8 + offset] = (stage2[3] + stage2[4]) / 8.f;
    out[4*8 + offset] = (stage2[3] - stage2[4]) / 8.f;
    out[5*8 + offset] = (stage2[2] - stage2[5]) / 8.f;
    out[6*8 + offset] = (stage2[1] - stage2[6]) / 8.f;
    out[7*8 + offset] = (stage2[0] - stage2[7]) / 8.f;
}
