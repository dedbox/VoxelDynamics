#pragma once

#include "VoxelDynamics/Vulkan/Buffer.hpp"

namespace VoxelDynamics::Vulkan
{

class UniformBuffer
{
public:
    vk::DeviceSize size;
    Buffer buffer;
    void* mapped;

    // proxy dereference operator
    Buffer& operator*() { return buffer; }
    const Buffer& operator*() const { return buffer; }
};

} // namespace VoxelDynamics::Vulkan
