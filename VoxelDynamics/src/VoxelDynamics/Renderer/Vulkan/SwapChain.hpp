#pragma once

#include "VoxelDynamics/Renderer/Vulkan/Device.hpp"
#include "VoxelDynamics/Renderer/Vulkan/Image.hpp"
#include "VoxelDynamics/Renderer/Vulkan/PhysicalDevice.hpp"

namespace VoxelDynamics::Vulkan
{

enum class ColorSpace
{
    SrgbNonlinear      = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    ExtendedSrgbLinear = VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT,
    Hdr10              = VK_COLOR_SPACE_HDR10_ST2084_EXT,
    Bt709Linear        = VK_COLOR_SPACE_BT709_LINEAR_EXT,
};

inline vk::ColorSpaceKHR to_vk(ColorSpace colorSpace)
{
    return static_cast<vk::ColorSpaceKHR>(colorSpace);
}

class SwapChain
{
public:
    vk::raii::SwapchainKHR vk_;
    vk::SurfaceFormatKHR surfaceFormat;
    ColorSpace colorSpace;
    std::vector<Image> images;
    std::vector<vk::Semaphore> acquireSemaphores;

    static SwapChain Create(
        const PhysicalDevice& physicalDevice,
        const Device& device,
        uint32_t width,
        uint32_t height,
        ColorSpace requestedColorSpace);

    void destroy();

    ~SwapChain() = default;

    SwapChain(SwapChain&&)            = default;
    SwapChain& operator=(SwapChain&&) = default;

    SwapChain(const SwapChain&)            = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    vk::raii::SwapchainKHR& operator*() { return vk_; }
    const vk::raii::SwapchainKHR& operator*() const { return vk_; }

    vk::raii::SwapchainKHR* operator->() { return &vk_; }
    const vk::raii::SwapchainKHR* operator->() const { return &vk_; }

private:
    SwapChain(
        vk::raii::SwapchainKHR swapChain,
        vk::SurfaceFormatKHR surfaceFormat,
        ColorSpace colorSpace,
        std::vector<Image> images,
        std::vector<vk::Semaphore> acquireSemaphores);

    static vk::SurfaceFormatKHR ChooseSurfaceFormat(
        const PhysicalDevice& physicalDevice, ColorSpace requestedColorSpace);
};

} // namespace VoxelDynamics::Vulkan
