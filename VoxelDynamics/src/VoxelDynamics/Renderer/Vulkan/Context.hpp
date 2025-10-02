#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Core/Window.hpp"
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
    vk::raii::Instance _instance;
    vk::raii::SurfaceKHR _surface;
    PhysicalDevice _physicalDevice;
    Device _device;

    // CreateInstance //////////////////////////////////////////////////////////////////////////////

    vk::raii::Instance createInstance(const CreateInfo& createInfo);

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugUtilsMessengerCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
        vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* /*pUserData*/);
    vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfoEXT();

    static std::vector<const char*> ValidationLayers();
    static std::vector<const char*> InstanceExtensions();

    static void CheckValidationLayers(const std::vector<const char*>& layers);
    static void CheckInstanceExtensions(const std::vector<const char*>& extensions);

    // CreateDevice ////////////////////////////////////////////////////////////////////////////////

    Device CreateDevice(const std::vector<const char*>& deviceExtensions);
};

} // namespace VoxelDynamics::Vulkan
