#pragma once

// #include "vulkan/vulkan_raii.hpp"

namespace VoxelDynamics::Vulkan
{

struct PipelineConfig
{
    // shader stages
    std::vector<char> spvCode;
    std::vector<vk::ShaderStageFlagBits> stages;
    std::vector<std::string> names;

    // fixed-function state
    vk::PipelineVertexInputStateCreateInfo vertexInpuState{};
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    // vk::PipelineTessellationStateCreateInfo tesselationState{};
    vk::PipelineRasterizationStateCreateInfo rasterizationState{};
    vk::PipelineMultisampleStateCreateInfo multisampleState{};
    // vk::PipelineDepthStencilStateCreateInfo depthStencilState{};
    vk::PipelineColorBlendStateCreateInfo colorBlendState{};

    // dynamic rendering state
    vk::PipelineRenderingCreateInfo renderingCreateInfo{};

    auto operator<=>(const VoxelDynamics::Vulkan::PipelineConfig&) const = default;
};

} // namespace VoxelDynamics::Vulkan

// golden ratio hashing
template <class T>
inline void hash_combine(size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
}

// vector hashing
template <typename T>
struct VectorHasher
{
    size_t operator()(const std::vector<T>& v) const noexcept
    {
        size_t h = 0;
        for (const T& i : v)
            h ^= std::hash<T>{}(i) + 0x9e3779b9 + (h << 6U) + (h >> 2U);
        return h;
    }
};

template <>
struct std::hash<vk::VertexInputBindingDescription>
{
    size_t operator()(const vk::VertexInputBindingDescription& v) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, v.binding);
        hash_combine(seed, v.stride);
        hash_combine(seed, v.inputRate);
        return seed;
    }
};

template <>
struct std::hash<vk::VertexInputAttributeDescription>
{
    size_t operator()(const vk::VertexInputAttributeDescription& v) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, v.location);
        hash_combine(seed, v.binding);
        hash_combine(seed, v.format);
        hash_combine(seed, v.offset);
        return seed;
    }
};

template <>
struct std::hash<vk::PipelineVertexInputStateCreateInfo>
{
    size_t operator()(const vk::PipelineVertexInputStateCreateInfo& v) const noexcept
    {
        size_t seed = 0;

        if (v.pVertexBindingDescriptions)
            for (uint32_t i = 0; i < v.vertexBindingDescriptionCount; ++i)
                hash_combine(seed, v.pVertexBindingDescriptions[i]);

        if (v.pVertexAttributeDescriptions)
            for (uint32_t i = 0; i < v.vertexAttributeDescriptionCount; ++i)
                hash_combine(seed, v.pVertexAttributeDescriptions[i]);

        return seed;
    }
};

template <>
struct std::hash<vk::PipelineInputAssemblyStateCreateInfo>
{
    size_t operator()(const vk::PipelineInputAssemblyStateCreateInfo& v) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, static_cast<uint32_t>(v.flags));
        hash_combine(seed, v.topology);
        hash_combine(seed, v.primitiveRestartEnable);
        return seed;
    }
};

template <>
struct std::hash<vk::PipelineRasterizationStateCreateInfo>
{
    size_t operator()(const vk::PipelineRasterizationStateCreateInfo& v) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, static_cast<uint32_t>(v.flags));
        hash_combine(seed, v.depthClampEnable);
        hash_combine(seed, v.rasterizerDiscardEnable);
        hash_combine(seed, v.polygonMode);
        hash_combine(seed, static_cast<uint32_t>(v.cullMode));
        hash_combine(seed, v.frontFace);
        hash_combine(seed, v.depthBiasEnable);
        hash_combine(seed, v.depthBiasConstantFactor);
        hash_combine(seed, v.depthBiasClamp);
        hash_combine(seed, v.depthBiasSlopeFactor);
        hash_combine(seed, v.lineWidth);
        return seed;
    }
};

template <>
struct std::hash<vk::PipelineMultisampleStateCreateInfo>
{
    size_t operator()(const vk::PipelineMultisampleStateCreateInfo& v) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, static_cast<uint32_t>(v.flags));
        hash_combine(seed, v.rasterizationSamples);
        hash_combine(seed, v.sampleShadingEnable);
        hash_combine(seed, v.minSampleShading);
        hash_combine(seed, v.pSampleMask);
        hash_combine(seed, v.alphaToCoverageEnable);
        hash_combine(seed, v.alphaToOneEnable);
        return seed;
    }
};

template <>
struct std::hash<vk::StencilOpState>
{
    size_t operator()(const vk::StencilOpState& v) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, v.failOp);
        hash_combine(seed, v.passOp);
        hash_combine(seed, v.depthFailOp);
        hash_combine(seed, v.compareOp);
        hash_combine(seed, v.compareMask);
        hash_combine(seed, v.writeMask);
        hash_combine(seed, v.reference);
        return seed;
    }
};

// template <>
// struct std::hash<vk::PipelineDepthStencilStateCreateInfo>
// {
//     size_t operator()(const vk::PipelineDepthStencilStateCreateInfo& v) const noexcept
//     {
//         size_t seed = 0;
//         hash_combine(seed, static_cast<uint32_t>(v.flags));
//         hash_combine(seed, v.depthTestEnable);
//         hash_combine(seed, v.depthWriteEnable);
//         hash_combine(seed, v.depthCompareOp);
//         hash_combine(seed, v.depthBoundsTestEnable);
//         hash_combine(seed, v.stencilTestEnable);
//         hash_combine(seed, v.front);
//         hash_combine(seed, v.back);
//         hash_combine(seed, v.minDepthBounds);
//         hash_combine(seed, v.maxDepthBounds);
//         return seed;
//     }
// };

template <>
struct std::hash<vk::PipelineColorBlendAttachmentState>
{
    size_t operator()(const vk::PipelineColorBlendAttachmentState& v) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, v.blendEnable);
        hash_combine(seed, v.srcColorBlendFactor);
        hash_combine(seed, v.dstColorBlendFactor);
        hash_combine(seed, v.colorBlendOp);
        hash_combine(seed, v.srcAlphaBlendFactor);
        hash_combine(seed, v.dstAlphaBlendFactor);
        hash_combine(seed, v.alphaBlendOp);
        hash_combine(seed, static_cast<uint32_t>(v.colorWriteMask));
        return seed;
    }
};

template <>
struct std::hash<vk::PipelineColorBlendStateCreateInfo>
{
    size_t operator()(const vk::PipelineColorBlendStateCreateInfo& v) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, static_cast<uint32_t>(v.flags));
        hash_combine(seed, v.logicOpEnable);

        if (v.pAttachments)
            for (uint32_t i = 0; i < v.attachmentCount; ++i)
                hash_combine(seed, v.pAttachments[i]);

        for (uint32_t i = 0; i < 4; ++i)
            hash_combine(seed, v.blendConstants[i]);

        return seed;
    }
};

template <>
struct std::hash<vk::PipelineRenderingCreateInfo>
{
    size_t operator()(const vk::PipelineRenderingCreateInfo& v) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, v.viewMask);
        hash_combine(seed, v.colorAttachmentCount);
        hash_combine(seed, v.pColorAttachmentFormats);
        hash_combine(seed, v.depthAttachmentFormat);
        hash_combine(seed, v.stencilAttachmentFormat);
        return seed;
    }
};

template <>
struct std::hash<VoxelDynamics::Vulkan::PipelineConfig>
{
    size_t operator()(const VoxelDynamics::Vulkan::PipelineConfig& k) const noexcept
    {
        size_t h = 0;

        // shader stages
        hash_combine(h, VectorHasher<char>{}(k.spvCode));

        for (const auto& stage : k.stages)
            hash_combine(h, stage);

        for (const auto& name : k.names)
            hash_combine(h, name);

        // fixed-function state
        hash_combine(h, k.vertexInpuState);
        hash_combine(h, k.inputAssemblyState);
        // hash_combind(h, k.tesselationState);
        hash_combine(h, k.rasterizationState);
        hash_combine(h, k.multisampleState);
        // hash_combine(h, k.depthStencilState);
        hash_combine(h, k.colorBlendState);

        // dynamic rendering state
        hash_combine(h, k.renderingCreateInfo);

        return h;
    }
};
