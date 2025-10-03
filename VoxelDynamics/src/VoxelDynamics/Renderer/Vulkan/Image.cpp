#include "VoxelDynamics/Renderer/Vulkan/Image.hpp"

namespace VoxelDynamics::Vulkan
{

Image::Image(vk::Image handle_, ImageType type_, vk::ImageView view_)
    : handle(handle_)
    , type(type_)
    , view(view_)
{
}

} // namespace VoxelDynamics::Vulkan
