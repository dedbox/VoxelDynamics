#include <VoxelDynamics.hpp>

#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"

// Sandbox App /////////////////////////////////////////////////////////////////////////////////////

using namespace VoxelDynamics;

struct Vertex
{
    glm::vec2 position;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        return vk::VertexInputBindingDescription(
            0,                             // binding index
            sizeof(Vertex),                // stride
            vk::VertexInputRate::eVertex); // input rate
    }

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        return {
            vk::VertexInputAttributeDescription(
                0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, position)),
            vk::VertexInputAttributeDescription(
                1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
        };
    }
};

class SandboxApp : public Application
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
    explicit SandboxApp(const Application::BuildInfo& buildInfo)
        : Application(buildInfo)
        , _pipeline(_context.createGraphicsPipeline(
              "shaders/slang.slang.spv",
              Vertex::getBindingDescription(),
              Vertex::getAttributeDescriptions()))
        , _frames(_context.createFrames())
        , _vertexBuffer(_context.createVertexBuffer(_frames, _vertices))
        , _indexBuffer(_context.createIndexBuffer(_frames, _indices))
    {
        // connect event listeners
        Event::Bus::Connect<Event::WindowClose, &SandboxApp::onClose>(this);
        Event::Bus::Connect<Event::WindowResuze, &SandboxApp::onResize>(this);
        Event::Bus::Connect<Event::KeyDown, &SandboxApp::onKeyDown>(this);

        // ready to run
        showWindow();
    }

    ~SandboxApp() override { _context.wait(); }

    // prevent copying
    SandboxApp(const SandboxApp&)            = delete;
    SandboxApp& operator=(const SandboxApp&) = delete;

    // prevent moving
    SandboxApp(SandboxApp&&)            = delete;
    SandboxApp& operator=(SandboxApp&&) = delete;

    void onClose() { quit(); }

    void onResize(const Event::WindowResuze& event)
    {
        Log::Info("Window resize event: {}x{}", event.width, event.height);
        const auto& [width, height] = getWWindowSize();
        if (width != event.width || height != event.height)
            _context.requestResize();
    }

    void onKeyDown(const VoxelDynamics::Event::KeyDown& event)
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

        VoxelDynamics::Log::Trace(
            "unhandled KeyDown event: {}{}", keyName, event.repeat ? " (repeat)" : "");
    }

    void onUpdate(double /*deltaTime*/) override
    {
        _context.drawCurrentFrame(_window, _frames, _pipeline, _vertexBuffer, _indexBuffer);
    }

private:
    Vulkan::Context::Pipeline _pipeline;
    Vulkan::Context::Frames _frames;
    Vulkan::Context::VertexBuffer _vertexBuffer;
    Vulkan::Context::IndexBuffer _indexBuffer;
};

// Sandbox App Builder /////////////////////////////////////////////////////////////////////////////

class SanndboxAppBuilder : public ApplicationBuilder
{
public:
    SanndboxAppBuilder()
    {
        _name      = "Sandbox";
        _placement = VoxelDynamics::CenteredWindowPlacement();
        _hidden    = true;
        _resizable = true;
        _logLevel  = VoxelDynamics::Log::Level::Debug;
    }

    // allow copying
    SanndboxAppBuilder(const SanndboxAppBuilder&)            = default;
    SanndboxAppBuilder& operator=(const SanndboxAppBuilder&) = default;

    // prevent moving
    SanndboxAppBuilder(SanndboxAppBuilder&&)            = delete;
    SanndboxAppBuilder& operator=(SanndboxAppBuilder&&) = delete;

    ~SanndboxAppBuilder() override = default;

    std::unique_ptr<Application> build() const override
    {
        return std::make_unique<SandboxApp>(GetBuildInfo());
    }
};

// Create Application //////////////////////////////////////////////////////////////////////////////

std::unique_ptr<VoxelDynamics::Application> VoxelDynamics::CreateApplication()
{
    auto app = SanndboxAppBuilder()
                   .version(0, 1, 0)
                   .title("Hello, world!")
                   .width(1280)
                   .height(720)
                   .build();
    return std::move(app);
}
