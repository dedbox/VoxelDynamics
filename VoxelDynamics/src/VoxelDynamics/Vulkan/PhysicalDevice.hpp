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
class PhysicalDevice
{
public:
    using FeaturesChain = vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>;

    struct QueueFamilyIndices
    {
        uint32_t transfer, graphics, present;
    };

    vk::raii::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceProperties properties;
    std::vector<std::string> availableExtensions;
    std::optional<QueueFamilyIndices> index;
    std::vector<uint32_t> uniqueIndices;
    std::vector<vk::SurfaceFormatKHR> surfaceFormats;
    std::vector<vk::PresentModeKHR> presentModes;
    std::optional<FeaturesChain> features;

    PhysicalDevice(vk::raii::PhysicalDevice physicalDevice, const vk::raii::SurfaceKHR& surface);

    ~PhysicalDevice() = default;

    // allow move
    PhysicalDevice(PhysicalDevice&&) noexcept            = default;
    PhysicalDevice& operator=(PhysicalDevice&&) noexcept = default;

    // prevent copy
    PhysicalDevice(const PhysicalDevice&)            = delete;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;

    // proxy dereference operator
    vk::raii::PhysicalDevice& operator*() { return physicalDevice; }
    const vk::raii::PhysicalDevice& operator*() const { return physicalDevice; }

    // proxy arrow operator
    vk::raii::PhysicalDevice* operator->() { return &physicalDevice; }
    const vk::raii::PhysicalDevice* operator->() const { return &physicalDevice; }

    std::vector<uint32_t> uniqueQueueFamilyIndices() const;

    static constexpr std::vector<const char*> RequiredDeviceExtensions()
    {
        return {
            vk::KHRSwapchainExtensionName,
        };
    }

    bool checkDeviceExtensions() const;

private:
    std::vector<std::string> getAvailableExtensions() const;

    std::optional<QueueFamilyIndices> findQueueFamilies(const vk::raii::SurfaceKHR& surface) const;

    std::optional<PhysicalDevice::FeaturesChain> createFeaturesChain();
};

} // namespace VoxelDynamics::Vulkan
