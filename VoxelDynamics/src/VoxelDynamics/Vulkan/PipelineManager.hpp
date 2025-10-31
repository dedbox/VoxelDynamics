#pragma once

#include "spirv_reflect.h"
#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Vulkan/Context.hpp"
#include "VoxelDynamics/Vulkan/PipelineConfig.hpp"

namespace VoxelDynamics::Vulkan
{

class PipelineManager
{
public:
    PipelineManager(const Context* context, vk::raii::PipelineCache pipelineCache);

    const vk::raii::Pipeline& getGraphicsPipeline(const PipelineConfig& config);
    const vk::raii::PipelineLayout& getPipelineLayout(const PipelineConfig& config);

private:
    const Context* _context;
    vk::raii::PipelineCache _pipelineCache;
    std::unordered_map<PipelineConfig, vk::raii::Pipeline> _pipelineCacheMap;
    std::unordered_map<PipelineConfig, vk::raii::PipelineLayout> _layoutCacheMap;

    static void CombineDescriptorSetBindings(
        std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>>& global_bindings,
        const SpvReflectDescriptorSet& reflect_set,
        vk::ShaderStageFlagBits stage);

    std::pair<vk::raii::PipelineLayout, std::vector<vk::raii::DescriptorSetLayout>>
    generatePipelineLayout(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::string& debugName) const;
};

} // namespace VoxelDynamics::Vulkan
