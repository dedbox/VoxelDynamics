#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Core/Window.hpp"

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
struct PhysicalDevice
{
    using FeaturesChain = vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>;

    vk::raii::PhysicalDevice physicalDevice;
    uint32_t graphicsIndex, presentIndex, transferIndex;
    std::vector<uint32_t> uniqueIndices;
    std::vector<vk::SurfaceFormatKHR> surfaceFormats;
    std::vector<vk::PresentModeKHR> presentModes;
    FeaturesChain features;

    // proxy dereference operator
    vk::raii::PhysicalDevice& operator*() { return physicalDevice; }
    const vk::raii::PhysicalDevice& operator*() const { return physicalDevice; }

    // proxy arrow operator
    vk::raii::PhysicalDevice* operator->() { return &physicalDevice; }
    const vk::raii::PhysicalDevice* operator->() const { return &physicalDevice; }
};

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

    const PhysicalDevice pickPhysicalDevice() const;

    static constexpr std::vector<const char*> DeviceExtensions();

    static bool CheckDeviceExtensions(const std::vector<std::string>&, const size_t i);

    static std::optional<PhysicalDevice::FeaturesChain> CreateFeaturesChain(
        const vk::raii::PhysicalDevice& physicalDevice, const size_t i);

    // Logical Device //////////////////////////////////////////////////////////////////////////////

    Device createDevice() const;
};

} // namespace VoxelDynamics::Vulkan
