#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace VoxelDynamics::Vulkan
{

class Surface
{
public:
    vk::raii::SurfaceKHR vk_;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

    static std::string FormatNames(const std::vector<vk::SurfaceFormatKHR>& formats);
    static std::string PresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes);

    vk::raii::SurfaceKHR& operator*() { return vk_; }
    const vk::raii::SurfaceKHR& operator*() const { return vk_; }

    vk::raii::SurfaceKHR* operator->() { return &vk_; }
    const vk::raii::SurfaceKHR* operator->() const { return &vk_; }
};

} // namespace VoxelDynamics::Vulkan
