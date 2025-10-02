#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Core/Window.hpp"

namespace VoxelDynamics::Vulkan
{

enum class DeviceType : uint8_t
{
    Discrete   = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    Integrated = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
    Virtual    = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
    Software   = VK_PHYSICAL_DEVICE_TYPE_CPU,
};

inline VkPhysicalDeviceType toNative(DeviceType type)
{
    return static_cast<VkPhysicalDeviceType>(type);
}

class Context
{
public:
    struct CreateInfo
    {
        std::string appName;
        uint32_t appVersion;
        DeviceType preferredDeviceType;
        std::vector<const char*> deviceExtensions;
    };

    Context(const Window& window, const CreateInfo& createInfo);

private:
    using FeaturesChain = vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features>;

    struct PhysicalDevice
    {
        vk::raii::PhysicalDevice handle;
        uint32_t graphicsQueueFamilyIndex;
        uint32_t computeQueueFamilyIndex;
        std::vector<vk::SurfaceFormatKHR> surfaceFormats;
        std::vector<vk::PresentModeKHR> surfacePresentModes;
        FeaturesChain features;
    };

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

    // CreateSurface ///////////////////////////////////////////////////////////////////////////////

    vk::raii::SurfaceKHR createSurface(const Window& window);

    // PickPhysicalDevice //////////////////////////////////////////////////////////////////////////

    PhysicalDevice pickPhysicalDevice(const CreateInfo& createInfo);

    static std::vector<const char*> DeviceExtensions(
        const std::vector<const char*>& deviceExtensions);

    static bool CheckDeviceExtensions(
        const vk::raii::PhysicalDevice& physicalDevice, const std::vector<const char*>& extensions);

    static std::optional<FeaturesChain> DeviceFeatures(
        const vk::raii::PhysicalDevice& physicalDevice);

    std::optional<std::pair<uint32_t, uint32_t>> findQueueFamilyIndices(
        const vk::raii::PhysicalDevice& physicalDevice);

    static std::string SurfaceFormatNames(const std::vector<vk::SurfaceFormatKHR>& formats);
    static std::string SufacePresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes);

    // CreateDevice ////////////////////////////////////////////////////////////////////////////////

    Device CreateDevice(const std::vector<const char*>& deviceExtensions);
};

} // namespace VoxelDynamics::Vulkan
