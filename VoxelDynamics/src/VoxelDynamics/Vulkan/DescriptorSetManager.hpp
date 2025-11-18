#pragma once

#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Vulkan/Context.hpp"

namespace VoxelDynamics::Vulkan
{

class DescriptorSetManager
{
public:
    DescriptorSetManager(const Context* context, uint32_t descriptorCount);

    std::vector<vk::raii::DescriptorSet> allocateDescriptorSets(
        const std::vector<vk::DescriptorSetLayout>& layouts) const;

private:
    const Context* _context;
    vk::raii::DescriptorPool _descriptorPool;

    vk::raii::DescriptorPool createDescriptorPool(uint32_t descriptorCount) const;
};

} // namespace VoxelDynamics::Vulkan
