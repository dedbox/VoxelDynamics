#include "VoxelDynamics/Core/Application.hpp"

#include "LVK.h"
#include "VoxelDynamics/Config.hpp"
#include "lvk/vulkan/VulkanUtils.h"

namespace VoxelDynamics
{

Application::Application(const CreateInfo& createInfo)
    : _window(CreateWindow(createInfo))
    , _context(lvk::createVulkanContextWithSwapchain(_window, 0, 0, {}))
{
    glfwSetKeyCallback(_window, [](GLFWwindow* window, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    });
}

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

GLFWwindow* Application::CreateWindow(const CreateInfo& createInfo)
{
    Log::Init(createInfo.name);
    Log::SetLevel(createInfo.logLevel);

    Log::Core::Info(
        "{} {}.{}.{}",
        EngineName,
        vk::versionMajor(EngineVersion),
        vk::versionMinor(EngineVersion),
        vk::versionPatch(EngineVersion));

    Log::Core::Info(
        "Starting application {}, version {}.{}.{}",
        createInfo.name,
        vk::versionMajor(createInfo.version),
        vk::versionMinor(createInfo.version),
        vk::versionPatch(createInfo.version));

    glfwSetErrorCallback([](int code, const char* description) {
        Log::Core::Error("GLFW Error ({}): {}", code, description);
    });

    Log::Core::Assert(glfwInit() == GLFW_TRUE, "glfwInit() failed");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, createInfo.resizable ? GLFW_TRUE : GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(
        static_cast<int>(createInfo.width),
        static_cast<int>(createInfo.height),
        createInfo.title.c_str(),
        nullptr,
        nullptr);

    Log::Core::Assert(window, "glfwCreateWindow() failed");

    std::visit(
        overloaded{
            [&](DefaultWindowPlacement) {
                // do nothing
            },

            [&](CenteredWindowPlacement) {
                // get dimensions of the screen
                const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

                //  calculate the position of the top-left corner
                const uint32_t x = (mode->width - createInfo.width) >> 1U;
                const uint32_t y = (mode->height - createInfo.height) >> 1U;

                // apply the calculated position
                glfwSetWindowPos(window, static_cast<int>(x), static_cast<int>(y));
            },

            [&](FixedWindowPlacement pos) {
                // apply the given position
                glfwSetWindowPos(window, static_cast<int>(pos.x), static_cast<int>(pos.y));
            },
        },
        createInfo.placement);

    return window;
}

Application::~Application()
{
    Log::Core::Info("Terminating application");
    glfwDestroyWindow(_window);
    glfwTerminate();
}

void Application::run() const
{
    int width = 0, height = 0;
    while (!glfwWindowShouldClose(_window))
    {
        glfwPollEvents();

        glfwGetFramebufferSize(_window, &width, &height);
        if (!(width && height))
            continue;

        lvk::ICommandBuffer& cmd = _context->acquireCommandBuffer();
        lvk::TextureHandle image = _context->getCurrentSwapchainTexture();

        vk::ClearValue clearColor({0.0F, 0.0F, 0.0F, 1.0F});
        vk::RenderingAttachmentInfo colorAttachmentInfo;
        colorAttachmentInfo.loadOp     = vk::AttachmentLoadOp::eClear;
        colorAttachmentInfo.clearValue = clearColor;

        vk::RenderingInfo renderInfo;
        renderInfo.setColorAttachments(colorAttachmentInfo);

        _context->submit(cmd, image);
    }
}

} // namespace VoxelDynamics
