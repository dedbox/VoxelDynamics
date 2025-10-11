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
};

extern std::unique_ptr<Application> CreateApplication();

} // namespace VoxelDynamics
