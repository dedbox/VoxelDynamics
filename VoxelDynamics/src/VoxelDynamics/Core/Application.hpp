#pragma once

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"

#include "VoxelDynamics/Core/Event.hpp"
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
        // context
        Vulkan::Context::BuildInfo context;
        // window
        std::string title;
        uint32_t width;
        uint32_t height;
        WindowPlacement placement;
        bool resizable;
        bool hidden;
        // application
        Log::Level logLevel;
        std::string identifier;
    } buildInfo;

    explicit Application(const BuildInfo& buildInfo);

    virtual ~Application();

    // allow moving
    Application(Application&&) noexcept            = default;
    Application& operator=(Application&&) noexcept = default;

    // prevent copying
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    // window management
    void showWindow() const;
    void hideWindow() const;

    // lifetime management
    bool isDone() const { return _isDone; }
    void quit() { _isDone = true; }

    void update();
    virtual void onUpdate(double deltaTime) {}

    void handleSdlEvent(SDL_Event* event);

private:
    SDL_Window* _window;
    Vulkan::Context _context;

    bool _isDone = false;
    double _lastFrameTime;

    static SDL_Window* CreateWindow(const BuildInfo& createInfo);

public:
    // Builder /////////////////////////////////////////////////////////////////////////////////////

    class Builder
    {
    public:
        virtual ~Builder() = default;

        const BuildInfo GetBuildInfo() const;

        virtual std::unique_ptr<Application> build() const;

        // application
        // ---------------------------------------------------------------------------------

        Builder& logLevel(Log::Level level)
        {
            _logLevel = level;
            return *this;
        }

        Builder& identifier(const std::string& identifier)
        {
            _identifier = identifier;
            return *this;
        }

        // window
        // --------------------------------------------------------------------------------------

        Builder& title(const std::string& title)
        {
            _title = title;
            return *this;
        }

        Builder& width(uint32_t width)
        {
            _width = width;
            return *this;
        }

        Builder& height(uint32_t height)
        {
            _height = height;
            return *this;
        }

        Builder& placement(WindowPlacement placement)
        {
            _placement = placement;
            return *this;
        }

        Builder& resizable(bool resizable)
        {
            _resizable = resizable;
            return *this;
        }

        Builder& hidden(bool hidden)
        {
            _hidden = hidden;
            return *this;
        }

        // context
        // -------------------------------------------------------------------------------------

        Builder& name(const std::string& name)
        {
            _name = name;
            return *this;
        }

        Builder& version(uint32_t major, uint32_t minor, uint32_t patch)
        {
            _version = Version(major, minor, patch);
            return *this;
        }

        Builder& preferredDeviceType(vk::PhysicalDeviceType type)
        {
            _preferredDeviceType = type;
            return *this;
        }

        // =============================================================================================

    protected:
        // window
        std::optional<std::string> _title = std::nullopt;
        uint32_t _width                   = 800;
        uint32_t _height                  = 600;
        bool _resizable                   = false;
        WindowPlacement _placement        = DefaultWindowPlacement();
        bool _hidden                      = false;

        // context
        std::string _name                           = "VxD-App";
        uint64_t _version                           = Version(1, 0, 0);
        vk::PhysicalDeviceType _preferredDeviceType = vk::PhysicalDeviceType::eDiscreteGpu;

        // application
        Log::Level _logLevel    = Log::Level::Info;
        std::string _identifier = std::format("com.voxeldynamics.{}", _name);
    };
};

extern std::unique_ptr<Application> CreateApplication();

} // namespace VoxelDynamics
