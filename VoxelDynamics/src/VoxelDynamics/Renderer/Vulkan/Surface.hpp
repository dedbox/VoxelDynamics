#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace VoxelDynamics::Vulkan
{

class Surface
{
public:
    vk::raii::SurfaceKHR handle;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

    static std::string FormatNames(const std::vector<vk::SurfaceFormatKHR>& formats);
    static std::string PresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes);

    const vk::raii::SurfaceKHR& operator*() { return handle; }
};

} // namespace VoxelDynamics::Vulkan
