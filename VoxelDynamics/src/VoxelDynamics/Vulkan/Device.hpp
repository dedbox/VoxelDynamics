#pragma once

#include "vulkan/vulkan_raii.hpp"

namespace VoxelDynamics::Vulkan
{

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
