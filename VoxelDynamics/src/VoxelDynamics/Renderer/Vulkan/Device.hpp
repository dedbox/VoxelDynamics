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

    vk::raii::Device vk_;
    vk::raii::Queue graphicsQueue;
    vk::raii::Queue computeQueue;

    vk::raii::Device& operator*() { return vk_; }
    const vk::raii::Device& operator*() const { return vk_; }

    vk::raii::Device* operator->() { return &vk_; }
    const vk::raii::Device* operator->() const { return &vk_; }

    void setDebugName(vk::ObjectType type, uint64_t handle, const std::string& name) const;

private:
    Device(vk::raii::Device device, vk::raii::Queue graphiceQueue, vk::raii::Queue computeQueue);
};

} // namespace VoxelDynamics::Vulkan
