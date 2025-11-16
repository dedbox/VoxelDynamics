#pragma once

#include <VoxelDynamics.hpp>

using namespace VoxelDynamics;

class SandboxApplication : public Application
{
public:
    explicit SandboxApplication(BuildInfo buildInfo)
        : Application(std::move(buildInfo))
    {
    }

protected:
    const Vulkan::Pipeline& createGraphicsPipeline(
        size_t vertexSize, const std::function<uint32_t(uint32_t)>& getLocationOffset)
    {
        Vulkan::PipelineConfig config;
        config.debugName = "Shader3 Pipeline";

        const auto spvCode = readFile("shaders/shader3.slang.spv");

        config.modules = {
            {.spirvBytecode  = spvCode,
             .stage          = vk::ShaderStageFlagBits::eVertex,
             .entryPointName = "vertMain"},
            {.spirvBytecode  = spvCode,
             .stage          = vk::ShaderStageFlagBits::eFragment,
             .entryPointName = "fragMain"},
        };

        config.vertexStride = vertexSize;

        config.inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo(
            {},                                   // flags
            vk::PrimitiveTopology::eTriangleList, // topology
            vk::False);                           // primitive restart enabled

        config.viewportState = vk::PipelineViewportStateCreateInfo(
            {},       // flags
            1,        // viewport count (must be 1 without muliViewport feature is enabled)
            nullptr,  // pViewports (ignored)
            1,        // scissor count (must match viewport count)
            nullptr); // pScissors (ignored)

        config.rasterizationState = vk::PipelineRasterizationStateCreateInfo(
            {},                               // flags
            vk::False,                        // depth clamp enabled
            vk::False,                        // rasterizer discard enabled
            vk::PolygonMode::eFill,           // polygon mode
            vk::CullModeFlagBits::eBack,      // cull mode
            vk::FrontFace::eCounterClockwise, // front face
            vk::False,                        // depth bias enabled
            0.0F,                             // depth bias constant factor
            0.0F,                             // depth bias clamp
            1.0F,                             // depth bias slope factor
            1.0F);                            // line width

        config.multisampleState = vk::PipelineMultisampleStateCreateInfo(
            {},                          // flags
            vk::SampleCountFlagBits::e1, // rasterization samples
            vk::False,                   // sample shading enabled
            0,                           // minimum sample shading
            nullptr,                     // sample mask
            vk::False,                   // alpha to coverage enabled
            vk::False);                  // alpha to one enabled

        vk::PipelineColorBlendAttachmentState colorBlendAttachment(
            vk::False,                       // blend enabled
            vk::BlendFactor::eZero,          // source color blend factor
            vk::BlendFactor::eZero,          // destination color blend factor
            vk::BlendOp::eAdd,               // color blend operation
            vk::BlendFactor::eZero,          // source alpha blend factor
            vk::BlendFactor::eZero,          // destination alpha blend factor
            vk::BlendOp::eAdd,               // alpha blend operation
            vk::ColorComponentFlagBits::eR | // color write mask
                vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA);

        config.colorBlendState = vk::PipelineColorBlendStateCreateInfo(
            {},                    // flags
            vk::False,             // logical operation enabled
            vk::LogicOp::eCopy,    // logical operation
            colorBlendAttachment); // color blend attachments

        config.renderingCreateInfo = vk::PipelineRenderingCreateInfo(
            0,                                             // view mask
            _renderer.getSwapChain().surfaceFormat.format, // color attachment formats
            vk::Format::eUndefined,                        // depth attachment format
            vk::Format::eUndefined);                       // stencil attachment format

        return _renderer.getPipelineManager().getGraphicsPipeline(config, getLocationOffset);
    }
};
