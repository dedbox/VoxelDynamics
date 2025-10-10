#include "VoxelDynamics/Core/Application.hpp"

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

namespace VoxelDynamics
{

// Application /////////////////////////////////////////////////////////////////////////////////////

Application::Application(const BuildInfo& buildInfo)
    : _appName(buildInfo.contextInfo.appName)
    , _window(CreateWindow(buildInfo))
    , _context(_window, buildInfo.contextInfo)
{
    glfwSetKeyCallback(_window, [](GLFWwindow* window, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    });

    glfwShowWindow(_window);
}

Application::~Application()
{
    Log::Core::Info("Terminating {}", _appName);
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
    }
}

GLFWwindow* Application::CreateWindow(const BuildInfo& appInfo)
{
    Log::Init(appInfo.contextInfo.appName);
    Log::SetLevel(appInfo.logLevel);

    Log::Core::Info(
        "{} {}.{}.{}",
        EngineName,
        vk::versionMajor(EngineVersion),
        vk::versionMinor(EngineVersion),
        vk::versionPatch(EngineVersion));

    Log::Core::Info(
        "Starting {}, version {}.{}.{}",
        appInfo.contextInfo.appName,
        vk::versionMajor(appInfo.contextInfo.appVersion),
        vk::versionMinor(appInfo.contextInfo.appVersion),
        vk::versionPatch(appInfo.contextInfo.appVersion));

    glfwSetErrorCallback([](int code, const char* description) {
        Log::Core::Error("GLFW Error ({}): {}", code, description);
    });

    if (glfwInit() != GLFW_TRUE)
        throw std::runtime_error("GLFW Error: initialzation failed");

    if (glfwVulkanSupported() == GLFW_FALSE)
        throw std::runtime_error("GLFW Error: Vulkan is not supported");

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, appInfo.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(
        static_cast<int>(appInfo.width),
        static_cast<int>(appInfo.height),
        appInfo.title.c_str(),
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
                const uint32_t x = (mode->width - appInfo.width) >> 1U;
                const uint32_t y = (mode->height - appInfo.height) >> 1U;

                // apply the calculated position
                glfwSetWindowPos(window, static_cast<int>(x), static_cast<int>(y));
            },

            [&](FixedWindowPlacement pos) {
                // apply the given position
                glfwSetWindowPos(window, static_cast<int>(pos.x), static_cast<int>(pos.y));
            },
        },
        appInfo.placement);

    return window;
}

// Application Builder /////////////////////////////////////////////////////////////////////////////

Application ApplicationBuilder::build() const
{
    const Application::BuildInfo buildInfo{
        .contextInfo =
            {
                .appName             = _name,
                .appVersion          = _version,
                .preferredDeviceType = _preferredDeviceType,
            },
        .title     = _title.value_or(_name),
        .width     = _width,
        .height    = _height,
        .placement = _placement,
        .logLevel  = _logLevel,
    };

    return Application(buildInfo);
}

void ApplicationBuilder::run() const
{
    Application app = build();
    app.run();
}

} // namespace VoxelDynamics
