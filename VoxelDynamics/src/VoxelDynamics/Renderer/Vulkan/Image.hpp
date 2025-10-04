#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace VoxelDynamics::Vulkan
{

enum class ImageType : uint8_t
{
    SwapChain,
    DepthMap,
    StencilMap
};

class Image
{
public:
    vk::Image vk_;
    ImageType type;
    vk::ImageView view;

    Image(vk::Image image, ImageType type, vk::ImageView view);

    vk::Image& operator*() { return vk_; }
    const vk::Image& operator*() const { return vk_; }

    vk::Image* operator->() { return &vk_; }
    const vk::Image* operator->() const { return &vk_; }
};

} // namespace VoxelDynamics::Vulkan
