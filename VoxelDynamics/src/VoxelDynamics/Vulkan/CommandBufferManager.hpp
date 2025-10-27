#pragma once

#include "VoxelDynamics/Vulkan/Context.hpp"
#include "VoxelDynamics/Vulkan/RenderQueue.hpp"

namespace VoxelDynamics::Vulkan
{

/** A Vulkan command pool and its allocated buffers.
 *
 * A CommandPoolAllocator handles the lifetime of a Vulkan command pool and manages the allocation
 * and resetting of its command buffers.
 */
class CommandPoolAllocator
{
public:
    CommandPoolAllocator(
        const Context* context,
        uint32_t queueFamilyIndex,
        vk::CommandPoolCreateFlags flags,
        const std::string& debugName);

    ~CommandPoolAllocator() = default;

    // allow move
    CommandPoolAllocator(CommandPoolAllocator&&) noexcept            = default;
    CommandPoolAllocator& operator=(CommandPoolAllocator&&) noexcept = default;

    // prevent copy
    CommandPoolAllocator(const CommandPoolAllocator&)            = delete;
    CommandPoolAllocator& operator=(const CommandPoolAllocator&) = delete;

    vk::raii::CommandBuffer allocateCommandBuffer(
        vk::CommandBufferLevel level, const std::string& debugName) const;

    void reset();

private:
    const Context* _context;
    vk::raii::CommandPool _commandPool;

    vk::raii::CommandPool createCommandPool(
        uint32_t queueFamilyIndex,
        vk::CommandPoolCreateFlags flags,
        const std::string& debugName) const;
};

/** Manages allocation and life cycle of Vulkan command buffers.
 *
 * The CommandBufferManager has separate CommandPoolAllocator objects for various queue family /
 * usage profile combinations.
 */
class CommandBufferManager
{
public:
    enum class UsageProfile : uint8_t
    {
        /** Graphics command buffers that are recorded once or infrequently. */
        GraphicsStatic,
        /** Graphics command buffers that are recorded every frame. */
        GraphicsDynamic,
        /** Transfer command buffers that are recorded once or infrequently. */
        TransferStatic,
        /** Transfer command buffers for repeated transfers within a frame. */
        TransferDynamic,
    };

    CommandBufferManager(const Context* context, uint32_t maxFramesInFlight);

    ~CommandBufferManager() = default;

    // prevent move
    CommandBufferManager(CommandBufferManager&&) noexcept            = delete;
    CommandBufferManager& operator=(CommandBufferManager&&) noexcept = delete;

    // prevent copy
    CommandBufferManager(const CommandBufferManager&)            = delete;
    CommandBufferManager& operator=(const CommandBufferManager&) = delete;

    vk::raii::CommandBuffer allocateOneShotBuffer(
        RenderQueue queue, vk::CommandBufferLevel level) const;

    vk::raii::CommandBuffer allocatePrimaryBuffer(UsageProfile profile, uint32_t frameIndex) const;

    vk::raii::CommandBuffer allocateSecondaryBuffer(
        UsageProfile profile, uint32_t frameIndex) const;

    void resetOneShotBuffers();
    void resetDynamicBuffers(uint32_t frameIndex);

    uint32_t getQueueFamilyIndex(RenderQueue queue) const;
    const vk::raii::Queue& getQueue(RenderQueue queue) const;

private:
    const Context* _context;

    CommandPoolAllocator _graphicsOncePool;
    CommandPoolAllocator _graphicsStaticPool;
    std::vector<CommandPoolAllocator> _graphicsDynamicPools;

    CommandPoolAllocator _transferOncePool;
    CommandPoolAllocator _transferStaticPool;
    std::vector<CommandPoolAllocator> _transferDynamicPools;

    std::vector<CommandPoolAllocator> createPoolAllocators(
        uint32_t count,
        uint32_t queueFamilyIndex,
        vk::CommandPoolCreateFlags flags,
        const std::string& debugName) const;

    vk::raii::CommandBuffer allocateBuffer(
        UsageProfile profile, uint32_t frameIndex, vk::CommandBufferLevel level) const;
};

} // namespace VoxelDynamics::Vulkan
