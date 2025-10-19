#pragma once

#include "VoxelDynamics/Core/Window.hpp"
#include "VoxelDynamics/Vulkan/Context.hpp"
#include "VoxelDynamics/Vulkan/SwapChain.hpp"

namespace VoxelDynamics::Vulkan
{

class Renderer
{
public:
    const struct BuildInfo
    {
        int maxFramesInFlight = 2;
    } buildInfo;

    Renderer(BuildInfo buildInfo, const Context* context, Window& window);

private:
    const Context* _context;
    SwapChain _swapChain;

    // Swap Chain //////////////////////////////////////////////////////////////////////////////////

    SwapChain createSwapChain(Window& window) const;

    void cleanupSwapChain();
};

} // namespace VoxelDynamics::Vulkan
