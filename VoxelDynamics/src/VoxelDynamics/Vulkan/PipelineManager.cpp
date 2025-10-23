#include "spirv_reflect.h"

#include "VoxelDynamics/Vulkan/PipelineManager.hpp"

namespace VoxelDynamics::Vulkan
{

std::pair<vk::raii::PipelineLayout, std::vector<vk::raii::DescriptorSetLayout>> PipelineManager::
    createLayout(const PipelineConfig& config) const
{
    std::vector<vk::raii::DescriptorSetLayout> setLayouts;
    std::vector<vk::PushConstantRange> pushConstantRanges;

    // discover bindings and push constant ranges
    for (const auto& spvCode : config.spvCodes)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;

        // analyze the shader module
        SpvReflectShaderModule module;
        SpvReflectResult result =
            spvReflectCreateShaderModule(spvCode.size(), spvCode.data(), &module);
        if (result != SPV_REFLECT_RESULT_SUCCESS)
            throw std::runtime_error("Could not reflect on SPIR-V module");

        // query descriptor sets
        uint32_t setCount = 0;
        spvReflectEnumerateDescriptorSets(&module, &setCount, nullptr);

        std::vector<SpvReflectDescriptorSet*> sets(setCount);
        spvReflectEnumerateDescriptorSets(&module, &setCount, sets.data());

        // discover bindings
        for (const auto& set : sets)
        {
            for (const auto& binding :
                 std::span<SpvReflectDescriptorBinding*>(set->bindings, set->binding_count))
            {
                const auto descriptorCount =
                    std::ranges::fold_left(binding->array.dims, 1, std::multiplies<>{});

                vk::DescriptorSetLayoutBinding layoutBinding(
                    binding->binding,
                    static_cast<vk::DescriptorType>(binding->descriptor_type),
                    descriptorCount,
                    static_cast<vk::ShaderStageFlagBits>(module.shader_stage),
                    nullptr);

                bindings.push_back(layoutBinding);
            }

            vk::DescriptorSetLayoutCreateInfo createInfo({}, set->binding_count, bindings.data());
            setLayouts.emplace_back(*_context->getDevice(), createInfo);
        }

        // query push constants
        uint32_t blockCount = 0;
        spvReflectEnumeratePushConstantBlocks(&module, &blockCount, nullptr);

        std::vector<SpvReflectBlockVariable*> blocks(blockCount);
        spvReflectEnumeratePushConstantBlocks(&module, &blockCount, blocks.data());

        // discover push constant ranges
        for (const auto& block : blocks)
            pushConstantRanges.emplace_back(
                static_cast<vk::ShaderStageFlagBits>(block->decoration_flags),
                block->offset,
                block->size);

        spvReflectDestroyShaderModule(&module);
    }

    // create pipeline layout
    std::vector<vk::DescriptorSetLayout> rawLayouts;
    rawLayouts.reserve(setLayouts.size());
    for (const auto& layout : setLayouts)
        rawLayouts.push_back(*layout);

    vk::PipelineLayoutCreateInfo createInfo({}, rawLayouts, pushConstantRanges);
    auto layout = vk::raii::PipelineLayout(*_context->getDevice(), createInfo);

    return std::make_pair(std::move(layout), std::move(setLayouts));
}

const vk::raii::PipelineLayout& PipelineManager::getPipelineLayout(const PipelineConfig& config)
{
    if (_layoutCacheMap.contains(config))
        return _layoutCacheMap.at(config);

    auto [layout, setLayouts] = createLayout(config);
    _layoutCacheMap.insert({config, std::move(layout)});

    return _layoutCacheMap.at(config);
}

const vk::raii::Pipeline& PipelineManager::getGraphicsPipeline(const PipelineConfig& config)
{
    if (_pipelineCacheMap.contains(config))
        return _pipelineCacheMap.at(config);

    // load shader modules
    std::vector<vk::raii::ShaderModule> modules;
    for (const auto& spvCode : config.spvCodes)
    {
        vk::ShaderModuleCreateInfo createInfo(
            {}, spvCode.size(), reinterpret_cast<const uint32_t*>(spvCode.data()));
        modules.emplace_back(*_context->getDevice(), createInfo);
    }

    // get pipeline layout
    std::vector<vk::PipelineShaderStageCreateInfo> stageInfos;
    for (const auto& [module, stage, name] :
         std::ranges::views::zip(modules, config.stages, config.names))
        stageInfos.emplace_back(vk::PipelineShaderStageCreateFlags{}, stage, *module, name.c_str());

    vk::PipelineLayout layout = getPipelineLayout(config);

    // dynamic states
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, dynamicStates);

    // color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    vk::PipelineColorBlendStateCreateInfo colorBlendState({}, {}, {}, colorBlendAttachment);

    // create graphics pipeline
    vk::GraphicsPipelineCreateInfo createInfo(
        {},
        stageInfos,
        &config.vertexInpuState,
        &config.inputAssemblyState,
        {}, // tesselation state
        {}, // viewport state
        &config.rasterizationState,
        &config.multisampleState,
        &config.depthStencilState,
        &colorBlendState,
        &dynamicStateInfo,
        layout,
        {},                           // rendering pass
        {},                           // subpass
        {},                           // base pipeline handle
        {},                           // base pipeline index
        &config.renderingCreateInfo); // pNext

    auto pipeline = _context->getDevice()->createGraphicsPipeline(_pipelineCache, createInfo);
    _pipelineCacheMap.insert({config, std::move(pipeline)});

    return _pipelineCacheMap.at(config);
}

} // namespace VoxelDynamics::Vulkan
