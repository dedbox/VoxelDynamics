#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "VoxelDynamics/Renderer/Vulkan/PhysicalDevice.hpp"

namespace VoxelDynamics::Vulkan
{

class Device
{
public:
    static Device Create(
        const PhysicalDevice& physicalDevice, const std::vector<const char*>& extraExtensions);

    vk::raii::Device handle;
    vk::raii::Queue graphicsQueue;
    vk::raii::Queue computeQueue;

    const vk::raii::Device& operator*() const { return handle; }

    void setDebugName(vk::ObjectType type, uint64_t handle, const std::string& name) const;

private:
    Device(vk::raii::Device device, vk::raii::Queue graphiceQueue, vk::raii::Queue computeQueue);
};

} // namespace VoxelDynamics::Vulkan
