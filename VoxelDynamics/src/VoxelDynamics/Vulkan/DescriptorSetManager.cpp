#include "VoxelDynamics/Vulkan/DescriptorSetManager.hpp"

namespace VoxelDynamics::Vulkan
{

DescriptorSetManager::DescriptorSetManager(const Context* context, uint32_t descriptorCount)
    : _context(context)
    , _descriptorPool(createDescriptorPool(descriptorCount))
{
}

vk::raii::DescriptorPool DescriptorSetManager::createDescriptorPool(uint32_t descriptorCount) const
{
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, descriptorCount);

    vk::DescriptorPoolCreateInfo poolInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, descriptorCount, poolSize);

    vk::raii::DescriptorPool pool(*_context->getDevice(), poolInfo);

    _context->setDebugName(vk::ObjectType::eDescriptorPool, &**pool, "Descriptor Pool");

    return pool;
}

std::vector<vk::raii::DescriptorSet> DescriptorSetManager::allocateDescriptorSets(
    const std::vector<vk::DescriptorSetLayout>& layouts) const
{
    vk::DescriptorSetAllocateInfo allocInfo(*_descriptorPool, layouts);

    return _context->getDevice()->allocateDescriptorSets(allocInfo);
}

} // namespace VoxelDynamics::Vulkan
