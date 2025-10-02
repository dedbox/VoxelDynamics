#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Core/Window.hpp"
#include "VoxelDynamics/Renderer/Vulkan/Device.hpp"
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
        uint32_t width;
        uint32_t height;
        PhysicalDeviceType preferredDeviceType;
        std::vector<const char*> deviceExtensions;
    };

    Context(const Window& window, const CreateInfo& createInfo);

private:
    vk::raii::Context _context;
    Instance _instance;
    PhysicalDevice _physicalDevice;
    Device _device;
};

} // namespace VoxelDynamics::Vulkan
