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
    struct BuildInfo
    {
        Vulkan::Context::BuildInfo contextInfo;
        std::string title;
        uint32_t width;
        uint32_t height;
        WindowPlacement placement;
        uint32_t x;
        uint32_t y;
        bool resizable;
        Log::Level logLevel;
    } buildInfo;

    Application();

    explicit Application(const BuildInfo& buildInfo);

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

    static GLFWwindow* CreateWindow(const BuildInfo& createInfo);
};

// Application Builder /////////////////////////////////////////////////////////////////////////////

class ApplicationBuilder
{
public:
    Application build() const;
    void run() const;

    // application ---------------------------------------------------------------------------------

    ApplicationBuilder& logLevel(Log::Level level)
    {
        _logLevel = level;
        return *this;
    }

    // window --------------------------------------------------------------------------------------

    ApplicationBuilder& title(const std::string& title)
    {
        _title = title;
        return *this;
    }

    ApplicationBuilder& width(uint32_t width)
    {
        _width = width;
        return *this;
    }

    ApplicationBuilder& height(uint32_t height)
    {
        _height = height;
        return *this;
    }

    ApplicationBuilder& placement(WindowPlacement placement)
    {
        _placement = placement;
        return *this;
    }

    // context -------------------------------------------------------------------------------------

    ApplicationBuilder& name(const std::string& name)
    {
        _name = name;
        return *this;
    }

    ApplicationBuilder& version(uint32_t major, uint32_t minor, uint32_t patch)
    {
        _version = Version(major, minor, patch);
        return *this;
    }

    ApplicationBuilder& preferredDeviceType(vk::PhysicalDeviceType type)
    {
        _preferredDeviceType = type;
        return *this;
    }

    // =============================================================================================

private:
    // window
    std::optional<std::string> _title = std::nullopt;
    uint32_t _width                   = 800;
    uint32_t _height                  = 600;
    WindowPlacement _placement        = DefaultWindowPlacement();

    // context
    std::string _name                           = "VxD Application";
    uint64_t _version                           = Version(1, 0, 0);
    vk::PhysicalDeviceType _preferredDeviceType = vk::PhysicalDeviceType::eDiscreteGpu;

    // application
    Log::Level _logLevel = Log::Level::Info;
};

} // namespace VoxelDynamics
