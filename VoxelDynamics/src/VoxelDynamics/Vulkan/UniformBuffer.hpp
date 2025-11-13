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

    UniformBuffer(vk::DeviceSize size, Buffer buffer, void* mapped)
        : size(size)
        , buffer(std::move(buffer))
        , mapped(mapped)
    {
    }

    ~UniformBuffer() = default;

    // allow move
    UniformBuffer(UniformBuffer&&)            = default;
    UniformBuffer& operator=(UniformBuffer&&) = default;

    // prevent copy
    UniformBuffer(const UniformBuffer&)            = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    // proxy dereference operator
    Buffer& operator*() { return buffer; }
    const Buffer& operator*() const { return buffer; }

    void update(const void* data) const { memcpy(mapped, data, size); }
};

} // namespace VoxelDynamics::Vulkan
