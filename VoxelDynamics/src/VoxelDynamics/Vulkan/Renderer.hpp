#pragma once

#include "VoxelDynamics/Core/Window.hpp"
#include "VoxelDynamics/Vulkan/Context.hpp"
#include "VoxelDynamics/Vulkan/SwapChain.hpp"

namespace VoxelDynamics::Vulkan
{

/** Uses a Context to construct a SwapChain and implement the high-level drawing logic.
 *
 * The Renderer is responsible for creating, resizing, and managing the life cycle of the
 * SwapChain. It creates a collection of Frame objects for coordinating multiple-frame-in-flight
 * rendering. In the main loop, the Renderer waits for the previous frame to finish, acquires the
 * next available SwapChain image, collects any CommandBuffer objects needed for the current frame
 * from the CommandBufferManager, and submits them to the graphics queue.
 */
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
