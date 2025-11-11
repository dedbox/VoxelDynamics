#include <VoxelDynamics.hpp>

// #include "VoxelDynamics/Core/Time.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/matrix_transform.hpp"
// #include "glm/trigonometric.hpp"

// Sandbox App /////////////////////////////////////////////////////////////////////////////////////

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

struct CameraBufferData
{
    glm::mat4 view;
    glm::mat4 projection;
};

struct ObjectBufferData
{
    glm::mat4 model;
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
        , _uniformBuffers(createUniformBuffers())
        , _descriptorPool(createDescriptorPool())
        , _descriptorSets(createDescriptorSets())
    {
    }

    void onCreate() override
    {
        const auto& device = _context.getDevice();

        // look forward and down at origin with a 45 degree angle
        _ubo_camera.view = glm::lookAt(
            glm::vec3(2.0F, 2.0F, 2.0F), glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(0.0F, 0.0F, 1.0F));

        // use perspective projection with 45 degree field of view
        const auto [width, height] = _window.getSize();
        _ubo_camera.projection     = glm::perspective(
            glm::radians(45.0F),
            static_cast<float>(width) / static_cast<float>(height),
            0.1F,
            10.0F);

        // undo glm's y-axis inversion
        _ubo_camera.projection[1][1] *= -1;

        // update camera uniform buffer
        for (const auto& buffers : _uniformBuffers)
            memcpy(buffers[0].mapped, &_ubo_camera, buffers[0].size);

        // configure descriptors
        for (const auto& [i, buffers, descriptorSet] :
             std::ranges::views::zip(std::ranges::views::iota(0), _uniformBuffers, _descriptorSets))
        {
            vk::DescriptorBufferInfo bufferInfo0(**buffers[0], 0, sizeof(CameraBufferData));
            vk::WriteDescriptorSet descriptorWrite0(
                descriptorSet, 0, 0, vk::DescriptorType::eUniformBuffer, {}, bufferInfo0);
            device->updateDescriptorSets(descriptorWrite0, {});

            vk::DescriptorBufferInfo bufferInfo1(**buffers[1], 0, sizeof(ObjectBufferData));
            vk::WriteDescriptorSet descriptorWrite1(
                descriptorSet, 1, 0, vk::DescriptorType::eUniformBuffer, {}, bufferInfo1);
            device->updateDescriptorSets(descriptorWrite1, {});
        }

        // connect event listeners
        Event::Bus::Connect<Event::WindowClose, &Sandbox::onClose>(this);
        Event::Bus::Connect<Event::WindowResuze, &Sandbox::onResize>(this);
        Event::Bus::Connect<Event::KeyDown, &Sandbox::onKeyDown>(this);

        // unhide the window
        _window.show();
    }

    void onClose() { quit(); }

    void onResize(const Event::WindowResuze& event)
    {
        Log::Info("Window resize event: {}x{}", event.width, event.height);
        const auto& [width, height] = _window.getSize();
        if (width != event.width || height != event.height)
            _renderer.requestResize();
    }

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
        const auto currentFrame    = _renderer.getSwapChain().currentFrame;
        const auto& uniformBuffers = _uniformBuffers[currentFrame];
        const auto& descriptorSets = _descriptorSets[currentFrame];

        // rotate 90 degrees per second around z-axis
        _ubo_object.model = glm::rotate(
            glm::mat4(1.0F),
            glm::radians(90.0F) * static_cast<float>(Time::Seconds()),
            glm::vec3(0.0F, 0.0F, 1.0F));

        // update object uniform buffer
        memcpy(uniformBuffers[1].mapped, &_ubo_object, sizeof(_ubo_object));

        _renderer.drawFrame(
            _window, *_graphicsPipeline, [&](const vk::raii::CommandBuffer& cmdBuffer) {
                // bind vertex data
                cmdBuffer.bindVertexBuffers(
                    0,                     // first binding
                    *_vertexBuffer.buffer, // buffer
                    {0});                  // offsets

                // bind index data
                cmdBuffer.bindIndexBuffer(
                    *_indexBuffer.buffer,    // buffer
                    0,                       // offset
                    vk::IndexType::eUint16); // index type

                // bind uniform data
                cmdBuffer.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,  // pipeline bind point
                    *_graphicsPipeline.pipelineLayout, // pipeline layout
                    0,                                 // first set
                    *descriptorSets,                   // descriptor sets
                    nullptr);                          // dynamic offsets

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
    CameraBufferData _ubo_camera{};
    ObjectBufferData _ubo_object{};

    const Vulkan::Pipeline& _graphicsPipeline;
    Vulkan::Buffer _vertexBuffer;
    Vulkan::Buffer _indexBuffer;

    std::vector<std::vector<Vulkan::UniformBuffer>> _uniformBuffers;

    vk::raii::DescriptorPool _descriptorPool;
    std::vector<vk::raii::DescriptorSet> _descriptorSets;

    const Vulkan::Pipeline& createGraphicsPipeline()
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

        return _renderer.getPipelineManager().getGraphicsPipeline(
            config, Vertex::getLocationOffset);
    }

    std::vector<std::vector<Vulkan::UniformBuffer>> createUniformBuffers() const
    {
        std::vector<std::vector<Vulkan::UniformBuffer>> uniformBuffers;
        uniformBuffers.reserve(buildInfo.renderer.maxFramesInFlight);

        for (const auto i : std::ranges::views::iota(0U, buildInfo.renderer.maxFramesInFlight))
        {
            std::vector<Vulkan::UniformBuffer> buffers;
            buffers.push_back(
                _context.createUniformBuffer(sizeof(CameraBufferData), "Camera Uniform Buffer"));
            buffers.push_back(
                _context.createUniformBuffer(sizeof(ObjectBufferData), "Object Uniform Buffer"));
            uniformBuffers.push_back(std::move(buffers));
        }

        return uniformBuffers;
    }

    vk::raii::DescriptorPool createDescriptorPool() const
    {
        vk::DescriptorPoolSize poolSize(
            vk::DescriptorType::eUniformBuffer, 2 * buildInfo.renderer.maxFramesInFlight);

        vk::DescriptorPoolCreateInfo poolInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            buildInfo.renderer.maxFramesInFlight,
            poolSize);

        vk::raii::DescriptorPool pool(*_context.getDevice(), poolInfo);

        _context.setDebugName(vk::ObjectType::eDescriptorPool, &**pool, "Descriptor Pool");

        return pool;
    }

    std::vector<vk::raii::DescriptorSet> createDescriptorSets() const
    {
        std::vector<vk::DescriptorSetLayout> layouts(
            buildInfo.renderer.maxFramesInFlight, *_graphicsPipeline.descriptorSetLayouts[0]);
        vk::DescriptorSetAllocateInfo allocInfo(*_descriptorPool, layouts);

        std::vector<vk::raii::DescriptorSet> descriptorSets =
            _context.getDevice()->allocateDescriptorSets(allocInfo);

        for (const auto& [i, descriptorSet] : std::ranges::views::enumerate(descriptorSets))
            _context.setDebugName(
                vk::ObjectType::eDescriptorSet,
                &**descriptorSet,
                std::format("Descriptor Set {}", i));

        return descriptorSets;
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
