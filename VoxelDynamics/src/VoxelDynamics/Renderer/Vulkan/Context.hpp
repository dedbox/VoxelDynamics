#pragma once

#include "GLFW/glfw3.h"
#include "vulkan/vulkan_raii.hpp"

namespace VoxelDynamics::Vulkan
{

class Context
{
public:
    struct CreateInfo
    {
        std::string appName                        = "VoxelDynamics Application";
        uint64_t appVersion                        = Version(1, 0, 0);
        vk::PhysicalDeviceType preferredDeviceType = vk::PhysicalDeviceType::eDiscreteGpu;
    };

    Context(GLFWwindow* window, const CreateInfo& contextInfo);

private:
    using FeaturesChain = vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>;

    vk::raii::Context _context;
    vk::raii::Instance _instance;
    vk::raii::SurfaceKHR _surface;

    struct PhysicalDevice
    {
        vk::raii::PhysicalDevice physicalDevice;
        uint32_t graphicsQueueFamilyIndex;
        std::vector<vk::SurfaceFormatKHR> surfaceFormats;
        std::vector<vk::PresentModeKHR> presentModes;
        FeaturesChain features;

        // dereference operator gives access to the underlying Vulkan object
        vk::raii::PhysicalDevice& operator*() { return physicalDevice; }
        const vk::raii::PhysicalDevice& operator*() const { return physicalDevice; }
    } _physicalDevice;

    // Instance ////////////////////////////////////////////////////////////////////////////////////

    vk::raii::Instance createInstance(const CreateInfo& contextInfo) const;

    static constexpr std::vector<const char*> InstanceLayers();
    static std::vector<const char*> InstanceExtensions();

    static void CheckInstanceLayers(const std::vector<const char*>& layers);
    static void CheckInstanceExtensions(const std::vector<const char*>& extensions);

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugUtilsMessengerCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
        vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* pUserData);

    static constexpr vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfoEXT();

    // Surface /////////////////////////////////////////////////////////////////////////////////////

    vk::raii::SurfaceKHR createSurface(GLFWwindow* window) const;

    // Physical Device /////////////////////////////////////////////////////////////////////////////

    PhysicalDevice pickPhysicalDevice(const vk::PhysicalDeviceType& preferredType) const;

    static constexpr std::vector<const char*> DeviceExtensions();

    static bool CheckDeviceExtensions(const std::vector<std::string>&, const size_t i);

    static std::string SurfaceFormatNames(const std::vector<vk::SurfaceFormatKHR>& formats);
    static std::string PresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes);

    static std::optional<FeaturesChain> CreateFeaturesChain(
        const vk::raii::PhysicalDevice& physicalDevice, const size_t i);
};

} // namespace VoxelDynamics::Vulkan
