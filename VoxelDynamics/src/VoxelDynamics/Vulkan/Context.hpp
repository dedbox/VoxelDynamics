#pragma once

#include "SDL3/SDL_video.h"
#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Vulkan/PhysicalDevice.hpp"

namespace VoxelDynamics::Vulkan
{

class Context
{
public:
    struct BuildInfo
    {
        vk::PhysicalDeviceType preferredDeviceType = vk::PhysicalDeviceType::eDiscreteGpu;
        int maxFramesInFlight                      = 2;
    } buildInfo;

    Context(
        BuildInfo buildInfo,
        const std::string& appName,
        const uint64_t appVersion,
        SDL_Window* window);

    ~Context() = default;

    // prevent move
    Context(Context&&) noexcept            = delete;
    Context& operator=(Context&&) noexcept = delete;

    // prevent copy
    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;

private:
    vk::raii::Context _context;
    vk::raii::Instance _instance;
    vk::raii::SurfaceKHR _surface;
    const PhysicalDevice _physicalDevice;

    // Instance ////////////////////////////////////////////////////////////////////////////////////

    vk::raii::Instance createInstance(const std::string& appName, const uint64_t appVersion) const;

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

    vk::raii::SurfaceKHR createSurface(SDL_Window* window) const;

    // Physical Device /////////////////////////////////////////////////////////////////////////////

    PhysicalDevice pickPhysicalDevice() const;

    static constexpr std::vector<const char*> DeviceExtensions();

    static bool CheckDeviceExtensions(const std::vector<std::string>&, const size_t i);

    static std::string SurfaceFormatName(const vk::SurfaceFormatKHR& format);
    static std::string SurfaceFormatNames(const std::vector<vk::SurfaceFormatKHR>& formats);
    static std::string PresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes);

    static std::optional<PhysicalDevice::FeaturesChain> CreateFeaturesChain(
        const vk::raii::PhysicalDevice& physicalDevice, const size_t i);
};

} // namespace VoxelDynamics::Vulkan
