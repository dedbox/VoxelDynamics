#pragma once

#include "vulkan/vulkan_raii.hpp"

namespace VoxelDynamics::Vulkan
{

struct SwapChain
{
    vk::raii::SwapchainKHR swapChain;
    std::vector<vk::Image> images;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::Extent2D extent;
    std::vector<vk::raii::ImageView> imageViews;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    bool frameBufferResized = false;
};

} // namespace VoxelDynamics::Vulkan
