#pragma once

// #include "vulkan/vulkan_raii.hpp"

// namespace VoxelDynamics::Vulkan
// {

// struct PhysicalDevice
// {
// public:
//     using FeaturesChain = vk::StructureChain<
//         vk::PhysicalDeviceFeatures2,
//         vk::PhysicalDeviceVulkan11Features,
//         vk::PhysicalDeviceVulkan12Features,
//         vk::PhysicalDeviceVulkan13Features>;

//     vk::raii::PhysicalDevice handle;
//     uint32_t graphicsQueueFamilyIndex;
//     uint32_t computeQueueFamilyIndex;
//     std::vector<vk::SurfaceFormatKHR> surfaceFormats;
//     std::vector<vk::PresentModeKHR> surfacePresentModes;
//     FeaturesChain features;

//     PhysicalDevice pickPhysicalDevice(const CreateInfo& createInfo);
// };

// } // namespace VoxelDynamics::Vulkan
