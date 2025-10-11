#include "VoxelDynamics/Core/Application.hpp"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"

#include "VoxelDynamics/Core/Event.hpp"
#include "VoxelDynamics/Core/EventBus.hpp"
#include "VoxelDynamics/Core/Time.hpp"

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

namespace VoxelDynamics
{

// Application /////////////////////////////////////////////////////////////////////////////////////

Application::Application(const BuildInfo& buildInfo_)
    : buildInfo(buildInfo_)
    , _window(CreateWindow(buildInfo_))
    , _context(_window, buildInfo_.context)
    , _lastFrameTime(Time::Seconds())
{
}

Application::~Application()
{
    Log::Core::Info("Terminating {}", buildInfo.context.appName);
}

SDL_Window* Application::CreateWindow(const BuildInfo& buildInfo)
{
    Log::Init(buildInfo.context.appName);
    Log::SetLevel(buildInfo.logLevel);

    Log::Core::Info(
        "{} {}.{}.{}",
        EngineName,
        vk::versionMajor(EngineVersion),
        vk::versionMinor(EngineVersion),
        vk::versionPatch(EngineVersion));

    const std::string appVersion = std::format(
        "{}.{}.{}",
        vk::versionMajor(buildInfo.context.appVersion),
        vk::versionMinor(buildInfo.context.appVersion),
        vk::versionPatch(buildInfo.context.appVersion));

    Log::Core::Info("Starting {}, version {}", buildInfo.context.appName, appVersion);

    const std::string cwd = std::filesystem::current_path();
    Log::Core::Info("Current working directory is {}", cwd);

    if (!SDL_Init(SDL_INIT_VIDEO))
        throw SDLException("Could not initialize SDL");

    if (!SDL_SetAppMetadata(
            buildInfo.context.appName.c_str(), appVersion.c_str(), buildInfo.identifier.c_str()))
        throw SDLException("Could not set application metadata");

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (buildInfo.resizable)
        flags |= SDL_WINDOW_RESIZABLE;
    if (buildInfo.hidden)
        flags |= SDL_WINDOW_HIDDEN;

    SDL_Window* window = SDL_CreateWindow(
        buildInfo.title.c_str(),
        static_cast<int>(buildInfo.width),
        static_cast<int>(buildInfo.height),
        flags);
    if (!window)
        throw SDLException("Could not create window");

    std::visit(
        overloaded{
            [&](DefaultWindowPlacement) {
                // do nothing
            },

            [&](CenteredWindowPlacement) {
                // determine the current screen
                const SDL_DisplayID displayId = SDL_GetDisplayForWindow(window);
                if (!displayId)
                    throw SDLException("Could not find the current screen");

                // get dimensions of the screen
                const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayId);
                if (!mode)
                    throw SDLException("Could not determine the current screen resolution");

                //  calculate the position of the top-left corner
                const uint32_t x = (mode->w - buildInfo.width) >> 1U;
                const uint32_t y = (mode->h - buildInfo.height) >> 1U;

                // apply the calculated position
                SDL_SetWindowPosition(window, static_cast<int>(x), static_cast<int>(y));
            },

            [&](FixedWindowPlacement pos) {
                // apply the given position directly
                SDL_SetWindowPosition(window, static_cast<int>(pos.x), static_cast<int>(pos.y));
            },
        },
        buildInfo.placement);

    return window;
}

// Window Management ///////////////////////////////////////////////////////////////////////////////

void Application::showWindow() const
{
    if (!SDL_ShowWindow(_window))
        throw SDLException("Could not show window");
}

void Application::hideWindow() const
{
    if (!SDL_HideWindow(_window))
        throw SDLException("Could not hide window");
}

// Lifetime Management /////////////////////////////////////////////////////////////////////////////

void Application::update()
{
    const double frameTime = Time::Seconds();
    const double deltaTime = frameTime - _lastFrameTime;
    _lastFrameTime         = frameTime;

    onUpdate(deltaTime);
}

// Event Handling //////////////////////////////////////////////////////////////////////////////////

void Application::handleSdlEvent(SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        Event::Bus::Trigger<Event::WindowClose>();
        break;

    case SDL_EVENT_KEY_DOWN:
        Event::Bus::Trigger<Event::KeyDown>(event->key.key, event->key.repeat);
        break;

    default:
        // unhandled
        break;
    }
}

} // namespace VoxelDynamics
