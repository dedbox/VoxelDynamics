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
    vk::Image handle;
    ImageType type;
    vk::ImageView view;

    Image(vk::Image handle, ImageType type, vk::ImageView view);
};

} // namespace VoxelDynamics::Vulkan
