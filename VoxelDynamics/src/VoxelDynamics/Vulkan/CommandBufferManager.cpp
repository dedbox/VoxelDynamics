#include "VoxelDynamics/Vulkan/CommandBufferManager.hpp"

namespace VoxelDynamics::Vulkan
{

// Command Pool Allocator //////////////////////////////////////////////////////////////////////////

CommandPoolAllocator::CommandPoolAllocator(
    const Context* context,
    uint32_t queueFamilyIndex,
    vk::CommandPoolCreateFlags flags,
    const std::string& debugName)
    : _context(context)
    , _commandPool(createCommandPool(queueFamilyIndex, flags, debugName))
{
}

vk::raii::CommandPool CommandPoolAllocator::createCommandPool(
    uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags, const std::string& debugName) const
{
    vk::raii::CommandPool cmdPool = _context->getDevice()->createCommandPool(
        vk::CommandPoolCreateInfo(flags, queueFamilyIndex));

    _context->setDebugName(vk::ObjectType::eCommandPool, &**cmdPool, debugName);

    return std::move(cmdPool);
}

vk::raii::CommandBuffer CommandPoolAllocator::allocateCommandBuffer(
    vk::CommandBufferLevel level, const std::string& debugName) const
{
    vk::CommandBufferAllocateInfo allocInfo(*_commandPool, level, 1);

    vk::raii::CommandBuffer cmdBuffer =
        std::move(_context->getDevice()->allocateCommandBuffers(allocInfo).front());

    _context->setDebugName(vk::ObjectType::eCommandBuffer, &**cmdBuffer, debugName);

    return std::move(cmdBuffer);
}

void CommandPoolAllocator::reset()
{
    if (*_commandPool)
    {
        _commandPool.reset();
        _allocatedBufers.clear();
    }
}

// Command Buffer Manager //////////////////////////////////////////////////////////////////////////

CommandBufferManager::CommandBufferManager(const Context* context, uint32_t maxFramesInFlight)
    : _context(context)
    , _graphicsStaticPool(
          context, context->getPhysicalDevice().graphicsIndex, {}, "Graphics Static Command Pool")
    , _graphicsDynamicPools(createPoolAllocators(
          maxFramesInFlight,
          context->getPhysicalDevice().graphicsIndex,
          vk::CommandPoolCreateFlagBits::eTransient,
          "Graphics Dynamic Command Pool"))
    , _transferStaticPool(
          context,
          context->getPhysicalDevice().transferIndex,
          vk::CommandPoolCreateFlagBits::eTransient,
          "Transfer Static Command Pool")
    , _transferDynamicPools(createPoolAllocators(
          maxFramesInFlight,
          context->getPhysicalDevice().transferIndex,
          vk::CommandPoolCreateFlagBits::eTransient,
          "Transfer Dynamic Command Pool"))
{
}

std::vector<CommandPoolAllocator> CommandBufferManager::createPoolAllocators(
    uint32_t count,
    uint32_t queueFamilyIndex,
    vk::CommandPoolCreateFlags flags,
    const std::string& debugName) const
{
    std::vector<CommandPoolAllocator> pools;
    pools.reserve(count);
    for (const auto i : std::ranges::views::iota(0U, count))
        pools.emplace_back(_context, queueFamilyIndex, flags, std::format("{} {}", debugName, i));

    return std::move(pools);
}

vk::raii::CommandBuffer CommandBufferManager::allocatePrimaryBuffer(
    UsageProfile profile, uint32_t frameIndex) const
{
    return allocateBuffer(profile, frameIndex, vk::CommandBufferLevel::ePrimary);
}

vk::raii::CommandBuffer CommandBufferManager::allocateSecondaryBuffer(
    UsageProfile profile, uint32_t frameIndex) const
{
    return allocateBuffer(profile, frameIndex, vk::CommandBufferLevel::eSecondary);
}

vk::raii::CommandBuffer CommandBufferManager::allocateBuffer(
    UsageProfile profile, uint32_t frameIndex, vk::CommandBufferLevel level) const
{
    switch (profile)
    {
    case UsageProfile::GraphicsStatic:
        return _graphicsStaticPool.allocateCommandBuffer(level, "Graphics Static Command Buffer");
    case UsageProfile::GraphicsDynamic:
        return _graphicsDynamicPools[frameIndex].allocateCommandBuffer(
            level, std::format("Grpahics Dynamic Command Buffer {}", frameIndex));
    case UsageProfile::TransferOnce:
        return _transferStaticPool.allocateCommandBuffer(level, "Transfer Static Command Buffer");
    case UsageProfile::TransferDynamic:
        return _transferDynamicPools[frameIndex].allocateCommandBuffer(
            level, std::format("Transfer Dynamic Command Buffer {}", frameIndex));
    }
    throw std::runtime_error("Unknown CommandBuffer::UsageProfile");
}

void CommandBufferManager::resetDynamicBuffers(uint32_t frameIndex)
{
    _graphicsDynamicPools[frameIndex].reset();
    _transferDynamicPools[frameIndex].reset();
}

} // namespace VoxelDynamics::Vulkan
