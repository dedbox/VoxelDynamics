#include "VoxelDynamics/Renderer/Vulkan/Surface.hpp"

namespace VoxelDynamics::Vulkan
{

std::string Surface::FormatNames(const std::vector<vk::SurfaceFormatKHR>& formats)
{
    const auto formatName = [](const vk::SurfaceFormatKHR& format) {
        return vk::to_string(format.format) + "+" + vk::to_string(format.colorSpace);
    };

    return formats | std::ranges::views::transform(formatName) |
           std::ranges::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>();
}

std::string Surface::PresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes)
{
    const auto modeName = [](const vk::PresentModeKHR& mode) { return vk::to_string(mode); };

    return presentModes | std::ranges::views::transform(modeName) |
           std::ranges::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>();
}

} // namespace VoxelDynamics::Vulkan
