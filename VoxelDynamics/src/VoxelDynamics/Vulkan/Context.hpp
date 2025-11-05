#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Core/Window.hpp"
#include "VoxelDynamics/Vulkan/Buffer.hpp"
#include "VoxelDynamics/Vulkan/Device.hpp"
#include "VoxelDynamics/Vulkan/Instance.hpp"
#include "VoxelDynamics/Vulkan/PhysicalDevice.hpp"

namespace VoxelDynamics::Vulkan
{

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

    Buffer createBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        const std::string& bufferName,
        const std::string& memoryName) const;

    void setDebugName(vk::ObjectType type, void* handle, const std::string& name) const;

    static std::string SurfaceFormatName(const vk::SurfaceFormatKHR& format);
    static std::string SurfaceFormatNames(const std::vector<vk::SurfaceFormatKHR>& formats);
    static std::string PresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes);

private:
    vk::raii::Context _context;
    const Instance _instance;
    vk::raii::SurfaceKHR _surface;
    const PhysicalDevice _physicalDevice;
    const Device _device;

    vk::raii::SurfaceKHR createSurface(Window& window) const;
    PhysicalDevice pickPhysicalDevice() const;

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
};

} // namespace VoxelDynamics::Vulkan
