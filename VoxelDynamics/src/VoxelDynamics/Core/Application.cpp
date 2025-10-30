#include "SDL3/SDL_init.h"

#include "VoxelDynamics/Core/Application.hpp"

#include "VoxelDynamics/Core/Event.hpp"
#include "VoxelDynamics/Core/EventBus.hpp"

namespace VoxelDynamics
{

Application::Application(BuildInfo buildInfo_)
    : buildInfo(initialize(std::move(buildInfo_)))
    , _window(Window(buildInfo.window))
    , _context(buildInfo.context, buildInfo.name, buildInfo.version, _window)
    , _renderer(buildInfo.renderer, &_context, _window)
{
}

Application::BuildInfo&& Application::initialize(BuildInfo&& buildInfo)
{
    // initialize logging subsystem
    Log::Init(buildInfo.name);
    Log::SetLevel(buildInfo.logLevel);

    // report engine version
    Log::Core::Info(
        "{} {}.{}.{}",
        EngineName,
        vk::versionMajor(EngineVersion),
        vk::versionMinor(EngineVersion),
        vk::versionPatch(EngineVersion));

    // report application version
    const std::string version = std::format(
        "{}.{}.{}",
        vk::versionMajor(buildInfo.version),
        vk::versionMinor(buildInfo.version),
        vk::versionPatch(buildInfo.version));
    Log::Core::Info("Starting {}, version {}", buildInfo.name, version);

    // report cwd
    const std::string cwd = std::filesystem::current_path();
    Log::Core::Info("Current working directory is {}", cwd);

    // initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO))
        throw SDLException("Could not initialize SDL");

    if (!SDL_SetAppMetadata(buildInfo.name.c_str(), version.c_str(), buildInfo.identifier.c_str()))
        throw SDLException("Could not set application metadata");

    return std::move(buildInfo);
}

Application::~Application()
{
    Log::Core::Info("Terminating {}", buildInfo.name);
    _renderer.wait();
}

void Application::handleSdlEvent(SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        Event::Bus::Trigger<Event::WindowClose>();
        break;

    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
        const auto& [width, height] = _window.getSize();
        Event::Bus::Trigger<Event::WindowResuze>(width, height);
        break;
    }

    case SDL_EVENT_KEY_DOWN:
        Event::Bus::Trigger<Event::KeyDown>(event->key.key, event->key.repeat);
        break;

    default:
        // unhandled
        break;
    }
}

} // namespace VoxelDynamics
