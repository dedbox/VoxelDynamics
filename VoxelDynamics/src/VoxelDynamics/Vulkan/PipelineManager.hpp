#pragma once

#include "spirv_reflect.h"
#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Vulkan/Context.hpp"
#include "VoxelDynamics/Vulkan/PipelineConfig.hpp"

namespace VoxelDynamics::Vulkan
{

struct Pipeline
{
    const vk::raii::Pipeline& pipeline;
    const vk::raii::PipelineLayout& layout;

    Pipeline(const vk::raii::Pipeline& pipeline_, const vk::raii::PipelineLayout& layout_)
        : pipeline(pipeline_)
        , layout(layout_)
    {
    }

    ~Pipeline() = default;

    // prevent move
    Pipeline(Pipeline&&)            = delete;
    Pipeline& operator=(Pipeline&&) = delete;

    // prevent copy
    Pipeline(const Pipeline&)            = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    // proxy dereference operator
    const vk::raii::Pipeline& operator*() const { return pipeline; }
};

class PipelineManager
{
public:
    PipelineManager(const Context* context, vk::raii::PipelineCache pipelineCache);

    Pipeline getGraphicsPipeline(const PipelineConfig& config);

private:
    const Context* _context;
    vk::raii::PipelineCache _pipelineCache;
    std::unordered_map<PipelineConfig, vk::raii::Pipeline> _pipelineCacheMap;
    std::unordered_map<PipelineConfig, vk::raii::PipelineLayout> _layoutCacheMap;

    const vk::raii::PipelineLayout& getPipelineLayout(const PipelineConfig& config);

    static void CombineDescriptorSetBindings(
        std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>>& global_bindings,
        const SpvReflectDescriptorSet& reflect_set,
        vk::ShaderStageFlagBits stage);

    std::pair<vk::raii::PipelineLayout, std::vector<vk::raii::DescriptorSetLayout>>
    generatePipelineLayout(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::string& debugName) const;

    static void LogShaderModuleConfig(
        const std::string& debugName,
        const ShaderModuleConfig& config,
        const SpvReflectEntryPoint* entryPoint);

    static std::string toString(SpvReflectDescriptorType type);
    static std::string arrayDimensions(SpvReflectArrayTraits& array);
    static std::string arrayDimensions(SpvReflectBindingArrayTraits& array);
};

} // namespace VoxelDynamics::Vulkan
