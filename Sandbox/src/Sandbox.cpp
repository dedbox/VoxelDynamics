#include <VoxelDynamics.hpp>

// #include "VoxelDynamics/Core/Time.hpp"
// #include "glm/ext/matrix_float4x4.hpp"
// #include "glm/ext/vector_float2.hpp"
// #include "glm/ext/vector_float3.hpp"
// #include "glm/gtc/matrix_transform.hpp"
// #include "glm/trigonometric.hpp"

// Sandbox App /////////////////////////////////////////////////////////////////////////////////////

// using namespace VoxelDynamics;

// struct Vertex
// {
//     glm::vec2 position;
//     glm::vec3 color;

//     static vk::VertexInputBindingDescription getBindingDescription()
//     {
//         return vk::VertexInputBindingDescription(
//             0,                             // binding index
//             sizeof(Vertex),                // stride
//             vk::VertexInputRate::eVertex); // input rate
//     }

//     static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
//     {
//         return {
//             vk::VertexInputAttributeDescription(
//                 0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, position)),
//             vk::VertexInputAttributeDescription(
//                 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
//         };
//     }
// };

// struct UniformBufferObject
// {
//     glm::mat4 model;
//     glm::mat4 view;
//     glm::mat4 projection;
// };

// class SandboxApp : public Application
// {
// private:
//     const std::vector<Vertex> _vertices = {
//         // clang-format off
//         {.position={-0.5F, -0.5F}, .color={1.0F, 0.0F, 0.0F}},
//         {.position={ 0.5F, -0.5F}, .color={0.0F, 1.0F, 0.0F}},
//         {.position={ 0.5F,  0.5F}, .color={0.0F, 0.0F, 1.0F}},
//         {.position={-0.5F,  0.5F}, .color={1.0F, 1.0F, 1.0F}},
//         // clang-format on
//     };

//     const std::vector<uint16_t> _indices = {0, 1, 2, 2, 3, 0};

// public:
//     explicit SandboxApp(const Application::BuildInfo& buildInfo)
//         : Application(buildInfo)
//         , _pipeline(_context.createGraphicsPipeline(
//               "shaders/shader.slang.spv",
//               Vertex::getBindingDescription(),
//               Vertex::getAttributeDescriptions(),
//               _context.createDescriptorSetLayout(
//                   vk::DescriptorSetLayoutBinding(
//                       0,                                  // binding
//                       vk::DescriptorType::eUniformBuffer, // descriptor type
//                       1,                                  // descriptor count
//                       vk::ShaderStageFlagBits::eVertex,   // shader stages
//                       nullptr)                            // immutable samplers
//                   )))
//         , _frames(_context.createFrames(_pipeline))
//         , _vertexBuffer(_context.createVertexBuffer(_frames, _vertices))
//         , _indexBuffer(_context.createIndexBuffer(_frames, _indices))
//         , _uniformBuffer(_context.createUniformBuffer<UniformBufferObject>(_frames))
//     {
//         // connect event listeners
//         Event::Bus::Connect<Event::WindowClose, &SandboxApp::onClose>(this);
//         Event::Bus::Connect<Event::WindowResuze, &SandboxApp::onResize>(this);
//         Event::Bus::Connect<Event::KeyDown, &SandboxApp::onKeyDown>(this);

//         // ready to run
//         showWindow();
//     }

//     ~SandboxApp() override { _context.wait(); }

//     // prevent copying
//     SandboxApp(const SandboxApp&)            = delete;
//     SandboxApp& operator=(const SandboxApp&) = delete;

//     // prevent moving
//     SandboxApp(SandboxApp&&)            = delete;
//     SandboxApp& operator=(SandboxApp&&) = delete;

//     void onClose() { quit(); }

//     void onResize(const Event::WindowResuze& event)
//     {
//         Log::Info("Window resize event: {}x{}", event.width, event.height);
//         const auto& [width, height] = getWWindowSize();
//         if (width != event.width || height != event.height)
//             _context.requestResize();
//     }

//     void onKeyDown(const VoxelDynamics::Event::KeyDown& event)
//     {
//         switch (event.key)
//         {
//         case SDLK_ESCAPE:
//             quit();
//             return;

//         default:
//             break;
//         }

//         const auto keyName = [&]() -> std::string {
//             const std::string name = SDL_GetKeyName(event.key);
//             return !name.empty() ? name : std::format("<key {}>", event.key);
//         }();

//         VoxelDynamics::Log::Trace(
//             "unhandled KeyDown event: {}{}", keyName, event.repeat ? " (repeat)" : "");
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

// private:
//     Vulkan::Context::Pipeline _pipeline;
//     Vulkan::Context::Frames _frames;
//     Vulkan::Context::VertexBuffer _vertexBuffer;
//     Vulkan::Context::IndexBuffer _indexBuffer;
//     std::vector<Vulkan::Context::UniformBuffer> _uniformBuffer;
// };

class Sandbox : public Application
{
public:
    using Application::Application;

    void onCreate() override
    {
        // connect event listeners
        Event::Bus::Connect<Event::WindowClose, &Sandbox::onClose>(this);
        Event::Bus::Connect<Event::KeyDown, &Sandbox::onKeyDown>(this);

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
};

// Sandbox App Builder /////////////////////////////////////////////////////////////////////////////

// class SanndboxAppBuilder : public ApplicationBuilder
// {
// public:
//     SanndboxAppBuilder()
//     {
//         _name      = "Sandbox";
//         _placement = VoxelDynamics::CenteredWindowPlacement();
//         _hidden    = true;
//         _resizable = true;
//         _logLevel  = VoxelDynamics::Log::Level::Debug;
//     }

//     // allow copying
//     SanndboxAppBuilder(const SanndboxAppBuilder&)            = default;
//     SanndboxAppBuilder& operator=(const SanndboxAppBuilder&) = default;

//     // prevent moving
//     SanndboxAppBuilder(SanndboxAppBuilder&&)            = delete;
//     SanndboxAppBuilder& operator=(SanndboxAppBuilder&&) = delete;

//     ~SanndboxAppBuilder() override = default;

//     std::unique_ptr<Application> build() const override
//     {
//         return std::make_unique<SandboxApp>(GetBuildInfo());
//     }
// };

// Entry Point /////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<VoxelDynamics::Application> VoxelDynamics::CreateApplication()
{
    const Application::BuildInfo buildInfo{
        .name     = "SandboxApp",
        .version  = Version(0, 1, 0),
        .logLevel = Log::Level::Info,
        .window =
            {
                .title     = "Sandbox",
                .width     = 1280,
                .height    = 720,
                .placement = Window::CenteredPlacement(),
                .resizable = true,
                .hidden    = true,
            },
    };

    return std::make_unique<Sandbox>(buildInfo);
}
