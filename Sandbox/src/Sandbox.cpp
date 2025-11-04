#include <VoxelDynamics.hpp>

// #include "VoxelDynamics/Core/Time.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
// #include "glm/gtc/matrix_transform.hpp"
// #include "glm/trigonometric.hpp"

// Sandbox App /////////////////////////////////////////////////////////////////////////////////////

//     void onResize(const Event::WindowResuze& event)
//     {
//         Log::Info("Window resize event: {}x{}", event.width, event.height);
//         const auto& [width, height] = getWWindowSize();
//         if (width != event.width || height != event.height)
//             _context.requestResize();
//     }

//     void onUpdate(double /*deltaTime*/) override
//     {
//         UniformBufferObject ubo{};

//         // rotate 90 degrees per second around z-axis
//         ubo.model = glm::rotate(
//             glm::mat4(1.0F),
//             glm::radians(90.0F) * static_cast<float>(VoxelDynamics::Time::Seconds()),
//             glm::vec3(0.0F, 0.0F, 1.0F));

//         // look forward and down at origin with a 45 degree angle
//         ubo.view = glm::lookAt(
//             glm::vec3(2.0F, 2.0F, 2.0F), glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(0.0F,
//             0.0F, 1.0F));

//         // use perspective projection with 45 degree field of view
//         const auto [width, height] = getWWindowSize();
//         ubo.projection             = glm::perspective(
//             glm::radians(45.0F),
//             static_cast<float>(width) / static_cast<float>(height),
//             0.1F,
//             10.0F);

//         // invert y-axis (for glm)
//         ubo.projection[1][1] *= -1;

//         _context.updateUniformBuffer(_frames, _uniformBuffer, ubo);
//         _context.drawCurrentFrame(
//             _window, _frames, _pipeline, _vertexBuffer, _indexBuffer, _uniformBuffer);
//     }

using namespace VoxelDynamics;

struct Vertex
{
    glm::vec2 position;
    glm::vec3 color;

    static uint32_t getLocationOffset(uint32_t location)
    {
        // clang-format off
        if (location == 0) return offsetof(Vertex, position);
        if (location == 1) return offsetof(Vertex, color);
        throw std::runtime_error("Unknown attribute location for offset mapping");
        // clang-format on
    }
};

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

class Sandbox : public Application
{
private:
    const std::vector<Vertex> _vertices = {
        // clang-format off
        {.position={-0.5F, -0.5F}, .color={1.0F, 0.0F, 0.0F}},
        {.position={ 0.5F, -0.5F}, .color={0.0F, 1.0F, 0.0F}},
        {.position={ 0.5F,  0.5F}, .color={0.0F, 0.0F, 1.0F}},
        {.position={-0.5F,  0.5F}, .color={1.0F, 1.0F, 1.0F}},
        // clang-format on
    };

    const std::vector<uint16_t> _indices = {0, 1, 2, 2, 3, 0};

public:
    explicit Sandbox(BuildInfo buildInfo)
        : Application(std::move(buildInfo))
        , _graphicsPipeline(createGraphicsPipeline())
        , _vertexBuffer(
              _renderer.transferVertexData(_vertices.data(), _vertices.size() * sizeof(Vertex)))
        , _indexBuffer(
              _renderer.transferIndexData(_indices.data(), _indices.size() * sizeof(uint16_t)))
    {
    }

    void onCreate() override
    {
        // connect event listeners
        Event::Bus::Connect<Event::WindowClose, &Sandbox::onClose>(this);
        // Event::Bus::Connect<Event::WindowResuze, &SandboxApp::onResize>(this);
        Event::Bus::Connect<Event::KeyDown, &Sandbox::onKeyDown>(this);

        // unhide the window
        _window.show();
    }

    void onClose() { quit(); }

    void onKeyDown(const Event::KeyDown& event)
    {
        switch (event.key)
        {
        case SDLK_ESCAPE:
            quit();
            return;

        default:
            break;
        }

        const auto keyName = [&]() -> std::string {
            const std::string name = SDL_GetKeyName(event.key);
            return !name.empty() ? name : std::format("<key {}>", event.key);
        }();

        Log::Trace("unhandled KeyDown event: {}{}", keyName, event.repeat ? " (repeat)" : "");
    }

    void onUpdate(double /*deltaTime*/) override
    {
        _renderer.drawFrame(
            _window, *_graphicsPipeline, [&](const vk::raii::CommandBuffer& cmdBuffer) {
                // bind vertex data
                cmdBuffer.bindVertexBuffers(
                    0,                     // first binding
                    *_vertexBuffer.buffer, // buffer
                    {0});                  // offsets

                // // bind uniform data
                // if (uniformBuffer)
                //     frame.gpBuffer.bindDescriptorSets(
                //         vk::PipelineBindPoint::eGraphics, // pipeline bind point
                //         *pipeline.pipelineLayout,         // pipeline layout
                //         0,                                // first set
                //         *frame.descriptorSet,             // descriptor sets
                //         nullptr);                         // dynamic offsets

                // cmdBuffer.bindDescriptorSets(
                //     vk::PipelineBindPoint::eGraphics,
                //     _graphicsPipeline.layout,
                //     0,
                //     const ArrayProxy<const vk::DescriptorSet>& descriptorSets,
                //     nullptr);

                // cmdBuffer.bindDescriptorSets(
                //     vk::PipelineBindPoint pipelineBindPoint,
                //     vk::PipelineLayout layout,
                //     uint32_t firstSet,
                //     const ArrayProxy<const vk::DescriptorSet>& descriptorSets,
                //     const ArrayProxy<const uint32_t> &dynamicOffsets)

                // bind index data
                cmdBuffer.bindIndexBuffer(
                    *_indexBuffer.buffer,    // buffer
                    0,                       // offset
                    vk::IndexType::eUint16); // index type

                // issue indexed draw command
                cmdBuffer.drawIndexed(
                    _indices.size(), // index count
                    1,               // instance count
                    0,               // first index
                    0,               // vertex offset
                    0);              // first instance

                // // issue non-indexed draw command
                // cmdBuffer.draw(
                //     _vertices.size(), // vertex count
                //     1,                // instance count
                //     0,                // first vertex
                //     0);               // first instance
            });
    }

private:
    const Vulkan::Pipeline& _graphicsPipeline;
    Vulkan::Buffer _vertexBuffer;
    Vulkan::Buffer _indexBuffer;

    const Vulkan::Pipeline& createGraphicsPipeline()
    {
        Vulkan::PipelineConfig config;
        config.debugName = "Shader2 Pipeline";

        const auto spvCode = readFile("shaders/shader2.slang.spv");

        config.modules = {
            {.spirvBytecode  = spvCode,
             .stage          = vk::ShaderStageFlagBits::eVertex,
             .entryPointName = "vertMain"},
            {.spirvBytecode  = spvCode,
             .stage          = vk::ShaderStageFlagBits::eFragment,
             .entryPointName = "fragMain"},
        };

        config.vertexStride = sizeof(Vertex);

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
            {},                          // flags
            vk::False,                   // depth clamp enabled
            vk::False,                   // rasterizer discard enabled
            vk::PolygonMode::eFill,      // polygon mode
            vk::CullModeFlagBits::eBack, // cull mode
            vk::FrontFace::eClockwise,   // front face
            vk::False,                   // depth bias enabled
            0.0F,                        // depth bias constant factor
            0.0F,                        // depth bias clamp
            1.0F,                        // depth bias slope factor
            1.0F);                       // line width

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

        return _renderer.getPipelineManager().getGraphicsPipeline(config, Vertex::getLocationOffset);
    }
};

// Entry Point /////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Application> VoxelDynamics::CreateApplication()
{
    const Application::BuildInfo buildInfo{
        .name     = "SandboxApp",
        .version  = Version(0, 1, 0),
        .logLevel = Log::Level::Debug,
        .window =
            {
                .title     = "Sandbox",
                .width     = 1280,
                .height    = 720,
                .placement = Window::CenteredPlacement(),
                .resizable = true,
                .hidden    = true,
            },
        .renderer = {
            .maxFramesInFlight = 2,
            .clearColor        = {0.0F, 0.0F, 0.0F, 1.0F},
        }};

    return std::make_unique<Sandbox>(buildInfo);
}
