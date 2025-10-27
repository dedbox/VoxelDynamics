#pragma once

namespace VoxelDynamics::Vulkan
{

enum class RenderQueue : uint8_t
{
    Graphics,
    // Compute,
    Transfer,
};

inline std::string toString(RenderQueue queue)
{
    switch (queue)
    {
    case RenderQueue::Graphics:
        return "Graphics";
    case RenderQueue::Transfer:
        return "Transfer";
    }
    throw std::runtime_error("Unknown render queue family");
}

} // namespace VoxelDynamics::Vulkan
