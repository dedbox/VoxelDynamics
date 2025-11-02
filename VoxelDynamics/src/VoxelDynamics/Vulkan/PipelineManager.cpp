#include "VoxelDynamics/Vulkan/PipelineManager.hpp"

namespace VoxelDynamics::Vulkan
{

PipelineManager::PipelineManager(const Context* context, vk::raii::PipelineCache pipelineCache)
    : _context(context)
    , _pipelineCache(std::move(pipelineCache))
{
}

const Pipeline& PipelineManager::getGraphicsPipeline(const PipelineConfig& config)
{
    // return cached pipeline, if it exists
    if (_pipelineCacheMap.contains(config))
        return _pipelineCacheMap.at(config);

    // otherwise, start creating a new pipeline
    auto [pipelineLayout, descriptorSetLayouts] =
        generatePipelineLayout(config.shaderModuleConfigs, config.debugName);

    // collect shader stage info
    std::vector<vk::PipelineShaderStageCreateInfo> stageInfos;
    stageInfos.reserve(config.shaderModuleConfigs.size());

    std::vector<vk::raii::ShaderModule> modules;
    for (const auto& shaderModuleConfig : config.shaderModuleConfigs)
    {
        vk::ShaderModuleCreateInfo moduleInfo(
            {},
            shaderModuleConfig.spirvBytecode.size(),
            reinterpret_cast<const uint32_t*>(shaderModuleConfig.spirvBytecode.data()));
        modules.emplace_back(*_context->getDevice(), moduleInfo);

        _context->setDebugName(
            vk::ObjectType::eShaderModule,
            &**modules.back(),
            std::format("Shader Module ({})", vk::to_string(shaderModuleConfig.stage)));

        vk::PipelineShaderStageCreateInfo stageInfo(
            {},
            shaderModuleConfig.stage,
            *modules.back(),
            shaderModuleConfig.entryPointName.c_str());

        stageInfos.push_back(stageInfo);
    }

    // configure dynamic states
    const std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, dynamicStates);

    // configure color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    vk::PipelineColorBlendStateCreateInfo colorBlendState({}, {}, {}, colorBlendAttachment);

    // create graphics pipeline
    vk::GraphicsPipelineCreateInfo createInfo(
        {},                           // flags
        stageInfos,                   // stages
        &config.vertexInputState,     // vertex input state
        &config.inputAssemblyState,   // input assembly state
        nullptr,                      // tesselation state
        &config.viewportState,        // viewport state
        &config.rasterizationState,   // rastrization state
        &config.multisampleState,     // multisample state
        nullptr,                      // depth/stencil state
        &colorBlendState,             // color blend state
        &dynamicStateInfo,            // dynamic states
        pipelineLayout,               // pipeline layout
        VK_NULL_HANDLE,               // rendering pass
        0,                            // subpass
        nullptr,                      // base pipeline handle
        -1,                           // base pipeline index
        &config.renderingCreateInfo); // pNext

    auto new_pipeline = _context->getDevice()->createGraphicsPipeline(_pipelineCache, createInfo);

    _context->setDebugName(vk::ObjectType::ePipeline, &**new_pipeline, config.debugName);

    Pipeline pipeline(std::move(new_pipeline), std::move(pipelineLayout));

    _pipelineCacheMap.insert({config, std::move(pipeline)});

    return _pipelineCacheMap.at(config);
}

void PipelineManager::CombineDescriptorSetBindings(
    std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>>& globalBindings,
    const SpvReflectDescriptorSet& set,
    vk::ShaderStageFlagBits stage)
{
    const auto bindings = std::span<SpvReflectDescriptorBinding*>(set.bindings, set.binding_count);

    for (const auto& binding : bindings)
    {
        uint32_t setNumber     = set.set;
        uint32_t bindingNumber = binding->binding;

        if (globalBindings.contains(setNumber) &&
            globalBindings.at(setNumber).contains(bindingNumber))
            // binding exists: add this stage to existing stages
            globalBindings.at(setNumber).at(bindingNumber).stageFlags |= stage;
        else
        {
            // new binding
            vk::DescriptorSetLayoutBinding new_binding(
                bindingNumber,
                static_cast<vk::DescriptorType>(binding->descriptor_type),
                binding->count,
                stage,
                nullptr);
            globalBindings[setNumber][bindingNumber] = new_binding;
        }
    }
}

std::pair<vk::raii::PipelineLayout, std::vector<vk::raii::DescriptorSetLayout>> PipelineManager::
    generatePipelineLayout(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::string& debugName) const
{
    std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>> globalBindings;
    std::vector<vk::PushConstantRange> pushConstantRanges;

    for (const auto& config : shaderModuleConfigs)
    {
        // create reflection module
        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule(
            config.spirvBytecode.size(), config.spirvBytecode.data(), &module);

        if (result != SPV_REFLECT_RESULT_SUCCESS)
            throw std::runtime_error("Could not create SPIRV-Reflect shader module");

        // find the entry point
        const SpvReflectEntryPoint* entryPoint =
            spvReflectGetEntryPoint(&module, config.entryPointName.c_str());

        if (entryPoint == nullptr)
            throw std::runtime_error("Could not find SPIR-V shader module entry point");

        LogEntryPoint(debugName, config, entryPoint);

        // combine descriptor sets
        const auto descriptorSets = std::span<SpvReflectDescriptorSet>(
            entryPoint->descriptor_sets, entryPoint->descriptor_set_count);

        for (const auto& [i, descriptorSet] : std::ranges::views::enumerate(descriptorSets))
            CombineDescriptorSetBindings(globalBindings, descriptorSet, config.stage);

        // find push constant blocks
        uint32_t numBlocks{};
        result = spvReflectEnumerateEntryPointPushConstantBlocks(
            &module, config.entryPointName.c_str(), &numBlocks, nullptr);

        if (result != SPV_REFLECT_RESULT_SUCCESS)
            throw std::runtime_error("Could not determine SPIR-V push constant block count");

        std::vector<SpvReflectBlockVariable*> blocks(numBlocks);
        result = spvReflectEnumerateEntryPointPushConstantBlocks(
            &module, config.entryPointName.c_str(), &numBlocks, blocks.data());

        if (result != SPV_REFLECT_RESULT_SUCCESS)
            throw std::runtime_error("Could not load SPIR-V push constant blocks");

        LogPushConstants(debugName, config, blocks);

        // determine push constant ranges
        for (const auto& [i, block] : std::ranges::views::enumerate(blocks))
            pushConstantRanges.emplace_back(config.stage, block->offset, block->size);

        // destroy the reflection module
        spvReflectDestroyShaderModule(&module);
    }

    // create descriptor set layouts
    std::vector<vk::raii::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<vk::DescriptorSetLayout> raw_descriptorSetLayouts;
    for (const auto& [i, pair] : std::ranges::views::enumerate(globalBindings))
    {
        const auto& [setNumber, setBindings] = pair;

        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        for (const auto& [bindingNumber, binding] : setBindings)
            bindings.push_back(binding);

        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings);
        descriptorSetLayouts.emplace_back(
            _context->getDevice()->createDescriptorSetLayout(layoutInfo));
        raw_descriptorSetLayouts.push_back(*descriptorSetLayouts.back());

        _context->setDebugName(
            vk::ObjectType::eDescriptorSetLayout,
            &**descriptorSetLayouts.back(),
            std::format("{} Descriptor Set Layout {}", debugName, i));
    }

    vk::PipelineLayoutCreateInfo createInfo({}, raw_descriptorSetLayouts, pushConstantRanges);
    vk::raii::PipelineLayout pipelineLayout(*_context->getDevice(), createInfo);

    _context->setDebugName(
        vk::ObjectType::ePipelineLayout, &**pipelineLayout, std::format("{} Layout", debugName));

    return std::make_pair(std::move(pipelineLayout), std::move(descriptorSetLayouts));
}

void PipelineManager::LogEntryPoint(
    const std::string& debugName,
    const ShaderModuleConfig& config,
    const SpvReflectEntryPoint* entryPoint)
{
    Log::Core::Info("Loading SPIR-V shader ({})", debugName);
    Log::Core::Info("  stage: {}", vk::to_string(config.stage));
    Log::Core::Info("  entry point: {}", config.entryPointName);

    const auto descriptorSets = std::span<SpvReflectDescriptorSet>(
        entryPoint->descriptor_sets, entryPoint->descriptor_set_count);

    Log::Core::Info(
        "{} > {} has {} descriptor set{}{}",
        debugName,
        config.entryPointName,
        descriptorSets.size(),
        descriptorSets.size() == 1 ? "" : "s",
        descriptorSets.size() == 0 ? "" : ":");

    for (const auto& [i, descriptorSet] : std::ranges::views::enumerate(descriptorSets))
    {
        std::span<SpvReflectDescriptorBinding*> bindings(
            descriptorSet.bindings, descriptorSet.binding_count);

        Log::Core::Info(
            "  set {} has {} binding{}{}",
            i,
            bindings.size(),
            bindings.size() == 1 ? "" : "s",
            bindings.size() == 0 ? "" : ":");

        for (const auto& [j, binding] : std::ranges::views::enumerate(bindings))
        {
            Log::Core::Info(
                "    binding {} is a{}:", binding->binding, toString(binding->descriptor_type));

            if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            {
                Log::Core::Info("      name: {}", binding->block.name);
                Log::Core::Info("      size: {} bytes", binding->block.size);
                Log::Core::Info("      member count: {}", binding->block.member_count);

                std::span<SpvReflectBlockVariable> members(
                    binding->block.members, binding->block.member_count);

                for (const auto& [i, member] : std::ranges::views::enumerate(members))
                {
                    Log::Core::Info("      member {}:", i);
                    Log::Core::Info("        name: {}", member.name);
                    Log::Core::Info("        type: {}", member.type_description->type_name);
                    Log::Core::Info("        byte offset: {}", member.offset);
                    Log::Core::Info("        size: {} bytes", member.size);

                    if (member.type_description->op == SpvOpTypeArray ||
                        member.type_description->op == SpvOpTypeRuntimeArray)
                        Log::Core::Info("        dimensions: {}", arrayDimensions(member.array));
                }
            }

            else if (binding->count == 0)
                Log::Core::Info("      array type: runtime", binding->binding, binding->name);

            else
            {
                Log::Core::Info("      array type: static");
                Log::Core::Info("      dimensions: {}", arrayDimensions(binding->array));
            }
        }
    }
}

void PipelineManager::LogPushConstants(
    const std::string& debugName,
    const ShaderModuleConfig& config,
    const std::vector<SpvReflectBlockVariable*>& blocks)
{
    Log::Core::Info(
        "{} > {} has {} push constant block{}",
        debugName,
        config.entryPointName,
        blocks.size(),
        blocks.size() == 1 ? "" : "s");

    for (const auto& [i, block] : std::ranges::views::enumerate(blocks))
        Log::Core::Info("  block {}: offset = {}, size = {}", i, block->offset, block->size);
}

std::string PipelineManager::toString(SpvReflectDescriptorType type)
{
    // clang-format off
    switch (type)
    {
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:                    return " sampler";
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:     return " combined image sampler";
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:              return " sampled image";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:              return " storage image";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:       return " uniform texel buffer";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:       return " storage texel buffer";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:             return " uniform buffer";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:             return " uniform storage buffer";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:     return " dynamic uniform buffer";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:     return " dynamic storage buffer";
    case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:           return "n input attachment";
    case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return "n acceleration structure";
    }
    // clang-format on
    std::unreachable();
}

std::string PipelineManager::arrayDimensions(SpvReflectArrayTraits& array)
{
    return std::span<uint32_t>(&array.dims[0], array.dims_count)                            //
           | std::ranges::views::transform([](uint32_t x) { return std::format("{}", x); }) //
           | std::ranges::views::join_with('x')                                             //
           | std::ranges::to<std::string>();
}

std::string PipelineManager::arrayDimensions(SpvReflectBindingArrayTraits& array)
{
    return std::span<uint32_t>(&array.dims[0], array.dims_count)                            //
           | std::ranges::views::transform([](uint32_t x) { return std::format("{}", x); }) //
           | std::ranges::views::join_with('x')                                             //
           | std::ranges::to<std::string>();
}

} // namespace VoxelDynamics::Vulkan
