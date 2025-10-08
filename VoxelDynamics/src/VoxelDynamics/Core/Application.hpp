#pragma once

#include "GLFW/glfw3.h"

#include "VoxelDynamics/Renderer/Vulkan/Context.hpp"

namespace VoxelDynamics
{

// Window Placement ////////////////////////////////////////////////////////////////////////////////

struct DefaultWindowPlacement
{
};

struct CenteredWindowPlacement
{
};

struct FixedWindowPlacement
{
    uint32_t x = 0;
    uint32_t y = 0;
};

using WindowPlacement =
    std::variant<DefaultWindowPlacement, CenteredWindowPlacement, FixedWindowPlacement>;

// Application ////////////////////////////////////////////////////////////////////////////////////

class Application
{
public:
    struct CreateInfo
    {
        Vulkan::Context::CreateInfo contextInfo;
        std::string title         = contextInfo.appName;
        uint32_t width            = 1280;
        uint32_t height           = 720;
        WindowPlacement placement = DefaultWindowPlacement();
        uint32_t x                = 0;
        uint32_t y                = 0;
        bool resizable            = false;
        Log::Level logLevel       = Log::Level::Info;
    };

    explicit Application(const CreateInfo& createInfo);

    ~Application();

    // prevent copying
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    // prevent moving
    Application(Application&&)            = delete;
    Application& operator=(Application&&) = delete;

    void run() const;

private:
    const std::string _appName;
    GLFWwindow* _window;
    Vulkan::Context _context;

    static GLFWwindow* CreateWindow(const CreateInfo& createInfo);
};

} // namespace VoxelDynamics
