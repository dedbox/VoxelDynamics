#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Core/Window.hpp"
#include "VoxelDynamics/Renderer/Vulkan/Instance.hpp"
#include "VoxelDynamics/Renderer/Vulkan/PhysicalDevice.hpp"

namespace VoxelDynamics::Vulkan
{

class Context
{
public:
    struct CreateInfo
    {
        std::string appName;
        uint32_t appVersion;
        PhysicalDeviceType preferredDeviceType;
        std::vector<const char*> deviceExtensions;
    };

    Context(const Window& window, const CreateInfo& createInfo);

private:
    struct Device
    {
        vk::raii::Device handle;
        vk::raii::Queue graphiceQueue;
        vk::raii::Queue computeQueue;
        vk::PhysicalDeviceFeatures features10;
        vk::PhysicalDeviceVulkan11Features features11;
        vk::PhysicalDeviceVulkan12Features features12;
        vk::PhysicalDeviceVulkan13Features features13;
    };

    vk::raii::Context _context;
    Instance _instance;
    vk::raii::SurfaceKHR _surface;
    PhysicalDevice _physicalDevice;
    Device _device;

    // CreateDevice ////////////////////////////////////////////////////////////////////////////////

    Device CreateDevice(const std::vector<const char*>& deviceExtensions);
};

} // namespace VoxelDynamics::Vulkan
