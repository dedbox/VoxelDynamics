#pragma once

#include "vulkan/vulkan_raii.hpp"

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

} // namespace VoxelDynamics::Vulkan
