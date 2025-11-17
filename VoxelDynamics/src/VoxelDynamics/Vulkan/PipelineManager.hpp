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
    vk::raii::PipelineLayout pipelineLayout;
    std::vector<vk::raii::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<vk::DescriptorSetLayout> raw_descriptorSetLayouts;

    Pipeline(
        vk::raii::Pipeline pipeline_,
        vk::raii::PipelineLayout pipelineLayout_,
        std::vector<vk::raii::DescriptorSetLayout> descriptorSetLayouts_,
        std::vector<vk::DescriptorSetLayout> raw_descriptorSetLayouts_)
        : pipeline(std::move(pipeline_))
        , pipelineLayout(std::move(pipelineLayout_))
        , descriptorSetLayouts(std::move(descriptorSetLayouts_))
        , raw_descriptorSetLayouts(std::move(raw_descriptorSetLayouts_))
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

    std::tuple<
        vk::raii::PipelineLayout,
        std::vector<vk::raii::DescriptorSetLayout>,
        std::vector<vk::DescriptorSetLayout>>
    generatePipelineLayout(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::vector<SpvReflectShaderModule>& modules,
        const std::vector<const SpvReflectEntryPoint*>& entryPoints,
        const std::string& debugName) const;

    std::pair<std::vector<vk::raii::DescriptorSetLayout>, std::vector<vk::DescriptorSetLayout>>
    generateDescriptorSetLayouts(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::vector<const SpvReflectEntryPoint*>& entryPoints,
        const std::string& debugName) const;

    std::vector<vk::PushConstantRange> generatePushConstantRanges(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::vector<SpvReflectShaderModule>& modules,
        const std::vector<const SpvReflectEntryPoint*>& entryPoints,
        const std::string& debugName) const;

    static void LogDescriptorSet(const std::string& prefix, const SpvReflectDescriptorSet& set);

    static void LogPushConstants(
        const std::string& debugName,
        const ShaderModuleConfig& config,
        const std::vector<SpvReflectBlockVariable*>& blocks);

    static std::string arrayDimensions(SpvReflectArrayTraits& array);
    static std::string arrayDimensions(SpvReflectBindingArrayTraits& array);
};

} // namespace VoxelDynamics::Vulkan
