#pragma once

#include "VoxelDynamics/Core/Window.hpp"
#include "VoxelDynamics/Vulkan/CommandBufferManager.hpp"
#include "VoxelDynamics/Vulkan/Context.hpp"
#include "VoxelDynamics/Vulkan/PipelineManager.hpp"
#include "VoxelDynamics/Vulkan/RenderQueue.hpp"

namespace VoxelDynamics::Vulkan
{

/** A container for the resources needed to present one image to the window surface.
 *
 * A SwapChainImageData object contains a render target image, a corresponding image view, and a
 * semaphore to prevent writing to the image while the presentation engine is still reading from it.
 */
struct SwapChainImageData
{
    vk::Image image;
    vk::raii::ImageView imageView;
    vk::raii::Semaphore renderFinishedSemaphore;
};

/** A container for the resources needed to process one frame of rendering work.
 *
 * Each FrameData object holds a semaphore to prevent multiple frames from writing to the same
 * SwapChain image at the same time, and a fence to signal when the frame's work is complete.
 */
struct FrameData
{
    vk::raii::CommandBuffer cmdBuffer;
    vk::raii::Semaphore imageAvailableSemaphore;
    vk::raii::Fence inFlightFence;
};

/** Manages a collection of images for presenting graphics to a surface.
 *
 * The SwapChain manages the resources needed to render images to the window surface. For each frame
 * in flight, the SwapChain provides an image as the render target for drawing commands. Once
 * drawing is complete, the SwapChain selects ts the final image for presentation on the window
 * surface.
 *
 * The SwapChain contains a read-only record of the chosen window surface format and extent, and a
 * collection of SwapChainImageData objects.
 *
 * When the window surface is resized or moved to a different device, the entire SwapChain must be
 * destroyed and recreated.
 */
struct SwapChain
{
    vk::raii::SwapchainKHR swapChain;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::Extent2D extent;
    bool resized;
    std::vector<SwapChainImageData> images;
    std::vector<FrameData> frames;
    uint32_t currentFrame;

    // proxy dereference operator
    vk::raii::SwapchainKHR& operator*() { return swapChain; }
    const vk::raii::SwapchainKHR& operator*() const { return swapChain; }

    // proxy arrow operator
    vk::raii::SwapchainKHR* operator->() { return &swapChain; }
    const vk::raii::SwapchainKHR* operator->() const { return &swapChain; }
};

/** A block of data allocated in physical memory.
 *
 * A Buffer object contains a Vulkan buffer handle and a device memory handle. The buffer handle
 * defines the size and intended usage of the buffer (e.g., vertex or index data). The device memory
 * handle represents an actual block of memory allocated from a specific memory heap on a physical
 * device such as in GPU RAM or a host-visible memory region.
 */
class Buffer
{
public:
    vk::raii::Buffer buffer;
    vk::raii::DeviceMemory memory;
};

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
        uint32_t maxFramesInFlight      = 2;
        std::array<float, 4> clearColor = {0.0F, 0.0F, 1.0F, 1.0F};
    } buildInfo;

    Renderer(BuildInfo buildInfo, const Context* context, const Window& window);

    ~Renderer() = default;

    // prevent move
    Renderer(Renderer&&) noexcept            = delete;
    Renderer& operator=(Renderer&&) noexcept = delete;

    // prevent copy
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    const SwapChain& getSwapChain() const { return _swapChain; }
    PipelineManager& getPipelineManager() { return _pipelineManager; }

    void requestResize() { _swapChain.resized = true; }

    void resetOneShotBuffers();
    void wait() const;

    Buffer transferVertexData(const void* data, size_t size) const;
    Buffer transferIndexData(const void* data, size_t size) const;

    using CommandRecorder = std::function<void(const vk::raii::CommandBuffer&)>;

    void drawFrame(
        const Window& window, const vk::raii::Pipeline& pipeline, const CommandRecorder& recorder);

private:
    const Context* _context;
    CommandBufferManager _cmdBufferManager;
    PipelineManager _pipelineManager;
    SwapChain _swapChain;

    // Swap Chain //////////////////////////////////////////////////////////////////////////////////

    void recreateSwapChain(const Window& window);
    SwapChain createSwapChain(const Window& window) const;
    void cleanupSwapChain();

    const FrameData& getCurrentFrameData() const;

    // Command Buffers /////////////////////////////////////////////////////////////////////////////

    Buffer createBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        const std::string& bufferName,
        const std::string& memoryName) const;

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

    Buffer createAndTransferBuffer(
        vk::BufferUsageFlagBits usage,
        RenderQueue destQueue,
        const vk::PipelineStageFlagBits2 stage,
        const vk::AccessFlagBits2 access,
        const void* data,
        size_t size,
        const std::string& bufferName,
        const std::string& memoryName) const;

    void recordCommandBuffer(
        uint32_t imageIndex,
        const vk::raii::Pipeline& pipeline,
        const CommandRecorder& recorder,
        const vk::raii::CommandBuffer& cmdBuffer);

    void transitionImageLayout(
        const vk::raii::CommandBuffer& buffer,
        vk::Image& image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask);
};

} // namespace VoxelDynamics::Vulkan
