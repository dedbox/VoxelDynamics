#include "VoxelDynamics/Vulkan/PipelineManager.hpp"

namespace VoxelDynamics::Vulkan
{

PipelineManager::PipelineManager(const Context* context, vk::raii::PipelineCache pipelineCache)
    : _context(context)
    , _pipelineCache(std::move(pipelineCache))
{
}

const Pipeline& PipelineManager::getGraphicsPipeline(
    const PipelineConfig& config, const std::function<uint32_t(uint32_t)>& getLocationOffset)
{
    auto&& [modules, entryPoints] = loadShaderModuleConfigs(config.modules, config.debugName);

    // create vertex input descriptions
    const auto vertexDescs = [&]() -> std::optional<std::pair<
                                       std::optional<vk::VertexInputBindingDescription>,
                                       std::vector<vk::VertexInputAttributeDescription>>> {
        auto it = std::ranges::find(
            config.modules, vk::ShaderStageFlagBits::eVertex, &ShaderModuleConfig::stage);

        if (it == config.modules.end())
            return std::nullopt;

        auto i = std::ranges::distance(config.modules.begin(), it);

        const auto [bindingDesc, attribDescs] = generateVertexInputDescriptions(
            modules[i], config.vertexStride, getLocationOffset, config.debugName);

        return std::make_pair(bindingDesc, attribDescs);
    }();

    const auto bindingDesc =
        vertexDescs.transform([](auto& p) { return p.first.value_or({}); }).value_or({});
    const auto attribDescs = vertexDescs.transform([](auto& p) { return p.second; }).value_or({});
    const auto vertexInputState =
        vk::PipelineVertexInputStateCreateInfo({}, bindingDesc, attribDescs);

    // return cached pipeline, if it exists
    if (_pipelineCacheMap.contains({config, vertexInputState}))
        return _pipelineCacheMap.at({config, vertexInputState});

    // otherwise, start creating a new pipeline
    std::vector<vk::PipelineShaderStageCreateInfo> stageInfos;
    stageInfos.reserve(config.modules.size());

    std::vector<vk::raii::ShaderModule> vk_modules;
    for (const auto& shaderModuleConfig : config.modules)
    {
        vk::ShaderModuleCreateInfo moduleInfo(
            {},
            shaderModuleConfig.spirvBytecode.size(),
            reinterpret_cast<const uint32_t*>(shaderModuleConfig.spirvBytecode.data()));
        vk_modules.emplace_back(*_context->getDevice(), moduleInfo);

        _context->setDebugName(
            vk::ObjectType::eShaderModule,
            &**vk_modules.back(),
            std::format("Shader Module ({})", vk::to_string(shaderModuleConfig.stage)));

        vk::PipelineShaderStageCreateInfo stageInfo(
            {},
            shaderModuleConfig.stage,
            *vk_modules.back(),
            shaderModuleConfig.entryPointName.c_str());

        stageInfos.push_back(stageInfo);
    }

    // create layouts
    auto [pipelineLayout, descriptorSetLayouts] =
        generatePipelineLayout(config.modules, modules, entryPoints, config.debugName);

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
        &vertexInputState,            // vertex input state
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

    _pipelineCacheMap.insert({{config, vertexInputState}, std::move(pipeline)});

    for (auto& module : modules)
        spvReflectDestroyShaderModule(&module);

    return _pipelineCacheMap.at({config, vertexInputState});
}

std::pair<std::vector<SpvReflectShaderModule>, std::vector<const SpvReflectEntryPoint*>>
PipelineManager::loadShaderModuleConfigs(
    const std::vector<ShaderModuleConfig>& shaderModuleConfigs, const std::string& debugName) const
{
    Log::Core::Info("Loading SPIR-V shader ({})", debugName);

    std::vector<SpvReflectShaderModule> modules;
    modules.reserve(shaderModuleConfigs.size());

    std::vector<const SpvReflectEntryPoint*> entryPoints;
    entryPoints.reserve(shaderModuleConfigs.size());

    for (const auto& config : shaderModuleConfigs)
    {
        // load module
        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule(
            config.spirvBytecode.size(), config.spirvBytecode.data(), &module);

        if (result != SPV_REFLECT_RESULT_SUCCESS)
            throw std::runtime_error("Could not create SPIRV-Reflect shader module");

        modules.push_back(module);

        // find entry point
        const SpvReflectEntryPoint* entryPoint =
            spvReflectGetEntryPoint(&module, config.entryPointName.c_str());

        if (entryPoint == nullptr)
            throw std::runtime_error("Could not find SPIR-V shader module entry point");

        LogEntryPoint(debugName, config, entryPoint);

        entryPoints.push_back(entryPoint);
    }

    return std::make_pair(modules, entryPoints);
}

std::pair<
    std::optional<vk::VertexInputBindingDescription>,
    std::vector<vk::VertexInputAttributeDescription>>
PipelineManager::generateVertexInputDescriptions(
    const SpvReflectShaderModule& module,
    size_t vertexStride,
    const std::function<uint32_t(uint32_t)>& getLocationOffset,
    const std::string& debugName) const
{
    // find all input variables
    uint32_t numVars{};
    spvReflectEnumerateInputVariables(&module, &numVars, nullptr);

    std::vector<SpvReflectInterfaceVariable*> all_vars(numVars);
    spvReflectEnumerateInputVariables(&module, &numVars, all_vars.data());

    // remove built-in variables
    auto user_vars =
        all_vars | std::views::filter([](const SpvReflectInterfaceVariable* var) -> bool {
            return var->built_in == -1;
        });

    // sort by location
    std::vector<SpvReflectInterfaceVariable*> vars(
        std::ranges::begin(user_vars), std::ranges::end(user_vars));

    std::ranges::sort(vars, std::less<uint32_t>{}, &SpvReflectInterfaceVariable::location);

    Log::Core::Info(
        "{} has {} user-defined vertex input variable{}:",
        debugName,
        vars.size(),
        vars.size() == 1 ? "" : "s");

    for (const auto& [i, var] : std::ranges::views::enumerate(vars))
    {
        Log::Core::Info("  variable {}:", i);
        Log::Core::Info("    name: {}", var->name);
        Log::Core::Info("    location: {}", var->location);
        Log::Core::Info("    format: {}", vk::to_string(static_cast<vk::Format>(var->format)));
        Log::Core::Info("    offset: {}", getLocationOffset(var->location));
    }

    // create binding description
    std::optional<vk::VertexInputBindingDescription> bindingDesc;
    if (vars.empty())
        bindingDesc = std::nullopt;
    else
    {
        bindingDesc = vk::VertexInputBindingDescription(
            0, // TODO support multiple vertex input bindings
            static_cast<uint32_t>(vertexStride),
            vk::VertexInputRate::eVertex);
    }

    // create attribute descriptions
    std::vector<vk::VertexInputAttributeDescription> attribDescs;
    attribDescs.reserve(vars.size());
    for (const auto& var : vars)
        attribDescs.emplace_back(
            var->location,
            0, // TODO support multiple vertex input bindings
            static_cast<vk::Format>(var->format),
            getLocationOffset(var->location));

    return std::make_pair(bindingDesc, attribDescs);
}

std::pair<vk::raii::PipelineLayout, std::vector<vk::raii::DescriptorSetLayout>> PipelineManager::
    generatePipelineLayout(
        const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
        const std::vector<SpvReflectShaderModule>& modules,
        const std::vector<const SpvReflectEntryPoint*>& entryPoints,
        const std::string& debugName) const
{
    std::vector<vk::raii::DescriptorSetLayout> descriptorSetLayouts =
        generateDescriptorSetLayouts(shaderModuleConfigs, entryPoints, debugName);

    std::vector<vk::PushConstantRange> pushConstantRanges =
        generatePushConstantRanges(shaderModuleConfigs, modules, entryPoints, debugName);

    // create pipeline layout
    const auto raw_descriptorSetLayouts =
        descriptorSetLayouts |
        std::ranges::views::transform([](const auto& layout) { return *layout; }) |
        std::ranges::to<std::vector<vk::DescriptorSetLayout>>();

    vk::PipelineLayoutCreateInfo createInfo({}, raw_descriptorSetLayouts, pushConstantRanges);
    vk::raii::PipelineLayout pipelineLayout(*_context->getDevice(), createInfo);

    _context->setDebugName(
        vk::ObjectType::ePipelineLayout, &**pipelineLayout, std::format("{} Layout", debugName));

    return std::make_pair(std::move(pipelineLayout), std::move(descriptorSetLayouts));
}

std::vector<vk::raii::DescriptorSetLayout> PipelineManager::generateDescriptorSetLayouts(
    const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
    const std::vector<const SpvReflectEntryPoint*>& entryPoints,
    const std::string& debugName) const
{
    std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>> globalBindings;

    for (const auto& [config, entryPoint] :
         std::ranges::views::zip(shaderModuleConfigs, entryPoints))
    {
        // combine descriptor sets
        const auto descriptorSets = std::span<SpvReflectDescriptorSet>(
            entryPoint->descriptor_sets, entryPoint->descriptor_set_count);

        for (const auto& [i, descriptorSet] : std::ranges::views::enumerate(descriptorSets))
        {
            const auto bindings = std::span<SpvReflectDescriptorBinding*>(
                descriptorSet.bindings, descriptorSet.binding_count);

            for (const auto& binding : bindings)
            {
                uint32_t setNumber     = descriptorSet.set;
                uint32_t bindingNumber = binding->binding;

                if (globalBindings.contains(setNumber) &&
                    globalBindings.at(setNumber).contains(bindingNumber))
                    globalBindings.at(setNumber).at(bindingNumber).stageFlags |= config.stage;

                else
                {
                    vk::DescriptorSetLayoutBinding new_binding(
                        bindingNumber,
                        static_cast<vk::DescriptorType>(binding->descriptor_type),
                        binding->count,
                        config.stage,
                        nullptr);
                    globalBindings[setNumber][bindingNumber] = new_binding;
                }
            }
        }
    }

    // create descriptor set layouts
    std::vector<vk::raii::DescriptorSetLayout> descriptorSetLayouts;
    for (const auto& [i, pair] : std::ranges::views::enumerate(globalBindings))
    {
        const auto& [setNumber, setBindings] = pair;

        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        for (const auto& [bindingNumber, binding] : setBindings)
            bindings.push_back(binding);

        vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings);
        descriptorSetLayouts.emplace_back(
            _context->getDevice()->createDescriptorSetLayout(layoutInfo));

        _context->setDebugName(
            vk::ObjectType::eDescriptorSetLayout,
            &**descriptorSetLayouts.back(),
            std::format("{} Descriptor Set Layout {}", debugName, i));
    }

    return descriptorSetLayouts;
}

std::vector<vk::PushConstantRange> PipelineManager::generatePushConstantRanges(
    const std::vector<ShaderModuleConfig>& shaderModuleConfigs,
    const std::vector<SpvReflectShaderModule>& modules,
    const std::vector<const SpvReflectEntryPoint*>& entryPoints,
    const std::string& debugName) const
{
    std::vector<vk::PushConstantRange> pushConstantRanges;

    for (const auto& [config, module, entryPoint] :
         std::ranges::views::zip(shaderModuleConfigs, modules, entryPoints))
    {
        // find push constant blocks
        uint32_t numBlocks{};
        SpvReflectResult result = spvReflectEnumerateEntryPointPushConstantBlocks(
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
    }

    return pushConstantRanges;
}

void PipelineManager::LogEntryPoint(
    const std::string& debugName,
    const ShaderModuleConfig& config,
    const SpvReflectEntryPoint* entryPoint)
{
    Log::Core::Info("Reflecting SPIR-V shader ({})", debugName);
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
