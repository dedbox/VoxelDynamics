#pragma once

#include "vulkan/vulkan_raii.hpp"

namespace VoxelDynamics::Renderer
{

class PhysicalDevice
{
public:
    using FeaturesChain = vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>;

    vk::raii::PhysicalDevice physicalDevice;
    uint32_t graphicsIndex, presentIndex, transferIndex;
    std::vector<uint32_t> uniqueIndices;
    std::vector<vk::SurfaceFormatKHR> surfaceFormats;
    std::vector<vk::PresentModeKHR> presentModes;
    FeaturesChain features;

    // proxy dereference operator
    vk::raii::PhysicalDevice& operator*() { return physicalDevice; }
    const vk::raii::PhysicalDevice& operator*() const { return physicalDevice; }

    // proxy arrow operator
    vk::raii::PhysicalDevice* operator->() { return &physicalDevice; }
    const vk::raii::PhysicalDevice* operator->() const { return &physicalDevice; }
};

} // namespace VoxelDynamics::Renderer
