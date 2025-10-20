#pragma once

#include "vulkan/vulkan_raii.hpp"

namespace VoxelDynamics::Vulkan
{

/** Manages a collection of images for presenting graphics to a surface.
 *
 * The SwapChain manages a collection of Frame objects holding the resources needed to render images
 * to the window surface as multiple "frames in flight." For each frame, the SwapChain provides an
 * image as the render target for drawing commands. Once drawing is complete, the SwapChain submits
 * the final image for presentation on the window surface.
 *
 * The SwapChain contains a read-only record of the chosen window surface format and extent, and a
 * collection of SwapChainImageData objects.
 *
 * When the window surface is resized or moved to a different device, the SwapChain must be
 * destroyed and recreated.
 */
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
