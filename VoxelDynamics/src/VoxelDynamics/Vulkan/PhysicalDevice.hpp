#pragma once

#include "vulkan/vulkan_raii.hpp"

namespace VoxelDynamics::Vulkan
{

/** A single, complete hardware implementation of Vulkan.
 *
 * A read-only object used during Context initialization to represent available hardware devices and
 * their capabilities. It primarily contains command queue family indices (graphics, present,
 * transfer), available surface formats, present modes (e.g., mailbox, immediate, FIFO), and
 * available Vulkan features such as support for geometry shaders, 64-bit floats, or specific
 * texture compression formats.
 */
struct PhysicalDevice
{
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

} // namespace VoxelDynamics::Vulkan
