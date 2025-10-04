#include "VoxelDynamics/Renderer/Vulkan/Image.hpp"

namespace VoxelDynamics::Vulkan
{

Image::Image(vk::Image image, ImageType type_, vk::ImageView view_)
    : vk_(image)
    , type(type_)
    , view(view_)
{
}

} // namespace VoxelDynamics::Vulkan
