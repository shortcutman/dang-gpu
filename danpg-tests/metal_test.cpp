//
//  metal_test.cpp
//  danpg-tests
//
//  Created by Daniel Burke on 10/1/2024.
//

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <strstream>

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "colour.hpp"

class MetalTest : public ::testing::Test {
protected:
    NS::SharedPtr<NS::AutoreleasePool> _pool;
    NS::SharedPtr<MTL::Device> _metalDevice;
    NS::SharedPtr<MTL::CommandQueue> _commandQueue;
    
    void SetUp() override {
        _pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
        _metalDevice = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
        ASSERT_TRUE(_metalDevice);
        
        _commandQueue = NS::TransferPtr(_metalDevice->newCommandQueue());
        ASSERT_TRUE(_commandQueue);
    }
    
    void TearDown() override {
        _metalDevice.reset();
        _pool.reset();
    }
    
    NS::SharedPtr<MTL::ComputePipelineState> newPSOFromFuncName(const char* functionName) {
        auto defaultLib = _metalDevice->newDefaultLibrary();
        EXPECT_TRUE(defaultLib) << "No default metal lib";
        
        auto function = defaultLib->newFunction(NS::String::string(functionName, NS::ASCIIStringEncoding));
        EXPECT_TRUE(function);
        
        NS::Error* error = nullptr;
        
        auto functionPSO = NS::TransferPtr(_metalDevice->newComputePipelineState(function, &error));
        EXPECT_TRUE(functionPSO);
        
        return functionPSO;
    }
};

namespace {

const unsigned int arrayLength = 1 << 24;
const unsigned int bufferSize = arrayLength * sizeof(float);

void generateRandomFloatData(MTL::Buffer* buffer) {
    float* data = reinterpret_cast<float*>(buffer->contents());
    
    for (unsigned int index = 0; index < arrayLength; index++) {
        data[index] = (float)rand() / (float)(RAND_MAX);
    }
}

}

TEST_F(MetalTest, AddArraysIsolatedTest) {
    auto pso = newPSOFromFuncName("add_arrays");
    EXPECT_TRUE(pso);
    
    NS::SharedPtr<MTL::Buffer> bufferA = NS::TransferPtr(_metalDevice->newBuffer(bufferSize, MTL::ResourceStorageModeShared));;
    NS::SharedPtr<MTL::Buffer> bufferB = NS::TransferPtr(_metalDevice->newBuffer(bufferSize, MTL::ResourceStorageModeShared));;
    NS::SharedPtr<MTL::Buffer> bufferResult = NS::TransferPtr(_metalDevice->newBuffer(bufferSize, MTL::ResourceStorageModeShared));;
    
    generateRandomFloatData(bufferA.get());
    generateRandomFloatData(bufferB.get());
    
    auto commandBuffer = _commandQueue->commandBuffer();
    assert(commandBuffer != nullptr);
    
    auto computeEncoder = commandBuffer->computeCommandEncoder();
    assert(computeEncoder != nullptr);
    
    computeEncoder->setComputePipelineState(pso.get());
    computeEncoder->setBuffer(bufferA.get(), 0, 0);
    computeEncoder->setBuffer(bufferB.get(), 0, 1);
    computeEncoder->setBuffer(bufferResult.get(), 0, 2);
    
    auto gridSize = MTL::Size(arrayLength, 1, 1);
    
    auto threadGroupSize = pso->maxTotalThreadsPerThreadgroup();
    if (threadGroupSize > arrayLength) {
        threadGroupSize = arrayLength;
    }
    
    auto threadGroupSizeObj = MTL::Size(threadGroupSize, 1, 1);
    
    computeEncoder->dispatchThreads(gridSize, threadGroupSizeObj);
    computeEncoder->endEncoding();
    
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
}

TEST_F(MetalTest, YCbCrToRGB) {
    auto pso = newPSOFromFuncName("ycbcrToRGB");
    EXPECT_TRUE(pso);
    
    NS::SharedPtr<MTL::Buffer> buffer = NS::TransferPtr(_metalDevice->newBuffer(sizeof(image::Colour), MTL::ResourceStorageModeShared));
    EXPECT_TRUE(sizeof(image::Colour) == 16);
    
    int* data = reinterpret_cast<int*>(buffer->contents());
    data[0] = -49;
    data[1] = -14;
    data[2] = 14;
    data[3] = 12345678;

    auto commandBuffer = _commandQueue->commandBuffer();
    assert(commandBuffer != nullptr);
    
    auto computeEncoder = commandBuffer->computeCommandEncoder();
    assert(computeEncoder != nullptr);
    
    computeEncoder->setComputePipelineState(pso.get());
    computeEncoder->setBuffer(buffer.get(), 0, 0);
    
    auto gridSize = MTL::Size(1, 1, 1);
    
    auto threadGroupSize = pso->maxTotalThreadsPerThreadgroup();
    if (threadGroupSize > arrayLength) {
        threadGroupSize = arrayLength;
    }
    
    auto threadGroupSizeObj = MTL::Size(threadGroupSize, 1, 1);
    
    computeEncoder->dispatchThreads(gridSize, threadGroupSizeObj);
    computeEncoder->endEncoding();
    
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
    
    EXPECT_EQ(data[0], 98);
    EXPECT_EQ(data[1], 73);
    EXPECT_EQ(data[2], 54);
    EXPECT_EQ(data[3], 12345678);
}
