#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Vulkan/PhysicalDevice.hpp"

namespace VoxelDynamics::Vulkan
{

/** The central object for all Vulkan device-level operations.
 *
 * A logical Device represents a direct interface to the chosen PhysicalDevice. It is used to issue
 * all Vulkan commands for creating resources (e.g., pipelines, command pools, images), allocating
 * command buffers, retrieving handles to queues, managing memory, and sumitting work.
 */
class Device
{
public:
    vk::raii::Device device;
    vk::raii::Queue graphicsQeeue;
    vk::raii::Queue presentQeeue;
    vk::raii::Queue transferQeeue;

    explicit Device(const PhysicalDevice& physicalDevice);

    ~Device() = default;

    // allow move
    Device(Device&&) noexcept            = default;
    Device& operator=(Device&&) noexcept = default;

    // prevent copy
    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;

    // proxy dereference operator
    vk::raii::Device& operator*() { return device; }
    const vk::raii::Device& operator*() const { return device; }

    // proxy arrow operator
    vk::raii::Device* operator->() { return &device; }
    const vk::raii::Device* operator->() const { return &device; }

private:
    static vk::raii::Device CreateDevice(const PhysicalDevice& physicalDevice);

    vk::raii::Queue createQueue(uint32_t index, const std::string& debugName) const;
};

} // namespace VoxelDynamics::Vulkan
