#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Renderer/Vulkan/Surface.hpp"

namespace VoxelDynamics::Vulkan
{

enum class PhysicalDeviceType : uint8_t
{
    Discrete   = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    Integrated = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
    Virtual    = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
    Software   = VK_PHYSICAL_DEVICE_TYPE_CPU,
};

inline vk::PhysicalDeviceType to_vk(PhysicalDeviceType type)
{
    return static_cast<vk::PhysicalDeviceType>(type);
}

class PhysicalDevice
{
public:
    using FeaturesChain = vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features>;

    vk::raii::PhysicalDevice handle;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t computeQueueFamilyIndex;
    Surface surface;
    FeaturesChain features;

    static PhysicalDevice Create(
        const vk::raii::Instance& instance,
        vk::raii::SurfaceKHR vk_surface,
        const PhysicalDeviceType preferredType,
        const std::vector<const char*>& extraExtensions);

    static std::vector<const char*> Extensions(const std::vector<const char*>& extraExtensions);

    const vk::raii::PhysicalDevice& operator*() const { return handle; }

private:
    PhysicalDevice(
        vk::raii::PhysicalDevice handle,
        uint32_t graphicsQueueFamilyIndex,
        uint32_t computeQueueFamilyIndex,
        Surface surface,
        const FeaturesChain& features);

    static bool CheckExtensions(
        const vk::raii::PhysicalDevice& physicalDevice, const std::vector<const char*>& extensions);

    static std::optional<std::pair<uint32_t, uint32_t>> findQueueFamilyIndices(
        const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface);

    static std::optional<FeaturesChain> Features(const vk::raii::PhysicalDevice& physicalDevice);
};

} // namespace VoxelDynamics::Vulkan
