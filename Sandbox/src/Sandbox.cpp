#include <VoxelDynamics.hpp>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "SandboxApplication.hpp"

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

class Sandbox : public SandboxApplication
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
        : SandboxApplication(std::move(buildInfo))
        , _graphicsPipeline(createGraphicsPipeline(sizeof(Vertex), Vertex::getLocationOffset))
        , _vertexBuffer(
              _renderer.transferVertexData(_vertices.data(), _vertices.size() * sizeof(Vertex)))
        , _indexBuffer(
              _renderer.transferIndexData(_indices.data(), _indices.size() * sizeof(uint16_t)))
        , _uniformDatas(createUniformDatas())
        , _descriptorSetManager(
              Vulkan::DescriptorSetManager(&_context, 3 * buildInfo.renderer.maxFramesInFlight))
        , _descriptorSets(createDescriptorSets())
        , _raw_descriptorSets(extractRawDescriptorSets())
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
        for (const auto& datas : _uniformDatas)
            datas[0].update(&_ubo_camera);

        // configure descriptor set updates
        for (const auto& [uniformBuffers, descriptorSets] :
             std::ranges::views::zip(_uniformDatas, _descriptorSets))
        {
            vk::DescriptorBufferInfo bufferInfo0(
                ****uniformBuffers[0], 0, sizeof(CameraBufferData));
            vk::WriteDescriptorSet descriptorWrite0(
                descriptorSets[0], 0, 0, vk::DescriptorType::eUniformBuffer, {}, bufferInfo0);
            device->updateDescriptorSets(descriptorWrite0, {});

            vk::DescriptorBufferInfo bufferInfo2(
                ****uniformBuffers[2], 0, sizeof(ObjectBufferData));
            vk::WriteDescriptorSet descriptorWrite2(
                descriptorSets[2], 0, 0, vk::DescriptorType::eUniformBuffer, {}, bufferInfo2);
            device->updateDescriptorSets(descriptorWrite2, {});
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
        const auto& uniformDatas   = _uniformDatas[currentFrame];
        const auto& descriptorSets = _raw_descriptorSets[currentFrame];

        // rotate 90 degrees per second around z-axis
        _ubo_object.model = glm::rotate(
            glm::mat4(1.0F),
            glm::radians(90.0F) * static_cast<float>(Time::Seconds()),
            glm::vec3(0.0F, 0.0F, 1.0F));

        // update object uniform buffer
        uniformDatas[2].update(&_ubo_object);

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
                    descriptorSets,                    // descriptor sets
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

    std::vector<std::vector<Vulkan::UniformData>> _uniformDatas;

    Vulkan::DescriptorSetManager _descriptorSetManager;

    std::vector<std::vector<vk::raii::DescriptorSet>> _descriptorSets;
    std::vector<std::vector<vk::DescriptorSet>> _raw_descriptorSets;

    std::vector<std::vector<Vulkan::UniformData>> createUniformDatas() const
    {
        const uint32_t maxFramesInFlight = buildInfo.renderer.maxFramesInFlight;

        // create one set of uniform buffers per frame
        std::vector<std::vector<Vulkan::UniformData>> uniformDatas(maxFramesInFlight);

        for (const auto i : std::ranges::views::iota(0U, maxFramesInFlight))
        {
            uniformDatas[i].emplace_back(
                &_context, sizeof(CameraBufferData), std::format("Camera (Frame {})", i));
            uniformDatas[i].emplace_back(
                &_context, std::nullopt, std::format("Unused (Frame {})", i));
            uniformDatas[i].emplace_back(
                &_context, sizeof(ObjectBufferData), std::format("Object (Frame {})", i));
        }

        return uniformDatas;
    }

    std::vector<std::vector<vk::raii::DescriptorSet>> createDescriptorSets() const
    {
        const uint32_t maxFramesInFlight = buildInfo.renderer.maxFramesInFlight;

        // create one copy of the raw layouts per frame
        std::vector<std::vector<vk::DescriptorSetLayout>> all_layouts;
        all_layouts.reserve(maxFramesInFlight);

        for (const auto _ : std::ranges::views::iota(0U, maxFramesInFlight))
            all_layouts.push_back(_graphicsPipeline.raw_descriptorSetLayouts);

        // allocate descriptor sets
        std::vector<std::vector<vk::raii::DescriptorSet>> all_descriptorSets;
        all_descriptorSets.reserve(maxFramesInFlight);

        for (const auto& [i, layouts] : std::ranges::views::enumerate(all_layouts))
        {
            all_descriptorSets.emplace_back(_descriptorSetManager.allocateDescriptorSets(layouts));

            for (const auto& [j, descriptorSet] :
                 std::ranges::views::enumerate(all_descriptorSets.back()))
                _context.setDebugName(
                    vk::ObjectType::eDescriptorSet,
                    &**descriptorSet,
                    std::format("Descriptor Set {} (Frame {})", j, i));
        }

        return all_descriptorSets;
    }

    std::vector<std::vector<vk::DescriptorSet>> extractRawDescriptorSets() const
    {
        return _descriptorSets //
               | std::views::transform([](const auto& descriptorSets) {
                     return descriptorSets //
                            | std::views::transform(
                                  [](const auto& descriptorSet) { return *descriptorSet; }) //
                            | std::ranges::to<std::vector>();
                 }) //
               | std::ranges::to<std::vector>();
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
