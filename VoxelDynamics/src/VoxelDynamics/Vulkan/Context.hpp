#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Core/Window.hpp"
#include "VoxelDynamics/Vulkan/PhysicalDevice.hpp"

namespace VoxelDynamics::Vulkan
{

/** The central object for all Vulkan device-level operations.
 *
 * A logical Device represents a direct interface to the chosen PhysicalDevice. It is used to issue
 * all Vulkan commands for creating resources (e.g., pipelines, command pools, images), allocating
 * command buffers, retrieving handles to queues, managing memory, and sumitting work.
 */
struct Device
{
public:
    vk::raii::Device device;
    vk::raii::Queue graphicsQeeue;
    vk::raii::Queue presentQeeue;
    vk::raii::Queue transferQeeue;

    // proxy dereference operator
    vk::raii::Device& operator*() { return device; }
    const vk::raii::Device& operator*() const { return device; }

    // proxy arrow operator
    vk::raii::Device* operator->() { return &device; }
    const vk::raii::Device* operator->() const { return &device; }
};

/** Performs all platform-specific and high-level Vulkan setup and manages Vulkan handles required
 * for interacting with the graphics hardware.
 *
 * The Context creates the Vulkan instance and window surface which form the bridge between Vulkan
 * and the native window system. It is responsible for enumerating available physical devices and
 * constructing a PhysicalDevice object for the most suitable one. The Context uses the chosen
 * PhysicalDevice to construct a logical Device and obtain queue handles for issuing graphics,
 * presentation, and bulk data transfer commands. It also provides basic functions for creating
 * Vulkan buffers and images.
 */
class Context
{
public:
    const struct BuildInfo
    {
        vk::PhysicalDeviceType preferredDeviceType = vk::PhysicalDeviceType::eDiscreteGpu;
    } buildInfo;

    Context(
        BuildInfo buildInfo, const std::string& appName, const uint64_t appVersion, Window& window);

    ~Context() = default;

    // prevent move
    Context(Context&&) noexcept            = delete;
    Context& operator=(Context&&) noexcept = delete;

    // prevent copy
    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;

    const vk::raii::SurfaceKHR& getSurface() const { return _surface; }
    const PhysicalDevice& getPhysicalDevice() const { return _physicalDevice; }
    const Device& getDevice() const { return _device; }

    void setDebugName(vk::ObjectType type, void* handle, const std::string& name) const;

    static std::string SurfaceFormatName(const vk::SurfaceFormatKHR& format);
    static std::string SurfaceFormatNames(const std::vector<vk::SurfaceFormatKHR>& formats);
    static std::string PresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes);

private:
    vk::raii::Context _context;
    vk::raii::Instance _instance;
    vk::raii::SurfaceKHR _surface;
    const PhysicalDevice _physicalDevice;
    const Device _device;

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

    vk::raii::SurfaceKHR createSurface(Window& window) const;

    // Physical Device /////////////////////////////////////////////////////////////////////////////

    PhysicalDevice pickPhysicalDevice() const;

    // Logical Device //////////////////////////////////////////////////////////////////////////////

    Device createDevice() const;
};

} // namespace VoxelDynamics::Vulkan
