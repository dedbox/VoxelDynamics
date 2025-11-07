#pragma once

#include "spirv_reflect.h"
#include "vulkan/vulkan_raii.hpp"

#include "VoxelDynamics/Vulkan/Context.hpp"
#include "VoxelDynamics/Vulkan/PipelineConfig.hpp"

namespace VoxelDynamics::Vulkan
{

struct Pipeline
{
    vk::raii::Pipeline pipeline;
    vk::raii::PipelineLayout layout;

    Pipeline(vk::raii::Pipeline pipeline_, vk::raii::PipelineLayout layout_)
        : pipeline(std::move(pipeline_))
        , layout(std::move(layout_))
    {
    }

    ~Pipeline() = default;

    // allow move
    Pipeline(Pipeline&&)            = default;
    Pipeline& operator=(Pipeline&&) = default;

    // prevent copy
    Pipeline(const Pipeline&)            = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    // proxy dereference operator
    vk::raii::Pipeline& operator*() { return pipeline; }
    const vk::raii::Pipeline& operator*() const { return pipeline; }
};

class PipelineManager
{
public:
    PipelineManager(const Context* context, vk::raii::PipelineCache pipelineCache);

    const Pipeline& getGraphicsPipeline(
        const PipelineConfig& config, const std::function<uint32_t(uint32_t)>& getLocationOffset);

private:
    const Context* _context;
    vk::raii::PipelineCache _pipelineCache;
    std::unordered_map<std::pair<PipelineConfig, vk::PipelineVertexInputStateCreateInfo>, Pipeline>
        _pipelineCacheMap;

    std::pair<std::vector<SpvReflectShaderModule>, std::vector<const SpvReflectEntryPoint*>>
    loadShaderModuleConfigs(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::string& debugName) const;

    std::pair<
        std::optional<vk::VertexInputBindingDescription>,
        std::vector<vk::VertexInputAttributeDescription>>
    generateVertexInputDescriptions(
        const SpvReflectShaderModule& module,
        size_t vertexStride,
        const std::function<uint32_t(uint32_t)>& getLocationOffset,
        const std::string& debugName) const;

    std::pair<vk::raii::PipelineLayout, std::vector<vk::raii::DescriptorSetLayout>>
    generatePipelineLayout(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::vector<SpvReflectShaderModule>& modules,
        const std::vector<const SpvReflectEntryPoint*>& entryPoints,
        const std::string& debugName) const;

    std::vector<vk::raii::DescriptorSetLayout> generateDescriptorSetLayouts(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::vector<const SpvReflectEntryPoint*>& entryPoints,
        const std::string& debugName) const;

    std::vector<vk::PushConstantRange> generatePushConstantRanges(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::vector<SpvReflectShaderModule>& modules,
        const std::vector<const SpvReflectEntryPoint*>& entryPoints,
        const std::string& debugName) const;

    static void LogEntryPoint(
        const std::string& debugName,
        const ShaderModuleConfig& config,
        const SpvReflectEntryPoint* entryPoint);

    static void LogPushConstants(
        const std::string& debugName,
        const ShaderModuleConfig& config,
        const std::vector<SpvReflectBlockVariable*>& blocks);

    static std::string toString(SpvReflectDescriptorType type);
    static std::string arrayDimensions(SpvReflectArrayTraits& array);
    static std::string arrayDimensions(SpvReflectBindingArrayTraits& array);
};

} // namespace VoxelDynamics::Vulkan
