#pragma once

#include "SDL3/SDL_events.h"

#include "VoxelDynamics/Core/Window.hpp"
#include "VoxelDynamics/Vulkan/Context.hpp"

namespace VoxelDynamics
{

class Application
{
public:
    const struct BuildInfo
    {
        std::string name       = "DefaultApp";
        uint64_t version       = Version(1, 0, 0);
        Log::Level logLevel    = Log::Level::Warn;
        std::string identifier = "com.voxeldynamics.default-app";
        Window::BuildInfo window{};
        Vulkan::Context::BuildInfo context{};
    } buildInfo;

    explicit Application(BuildInfo buildInfo);

    virtual ~Application();

    // prevent move
    Application(Application&&)            = delete;
    Application& operator=(Application&&) = delete;

    // prevent copy
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    // life cycle management
    void quit() { _done = true; }
    bool isDone() const { return _done; }

    virtual void onCreate() {}
    virtual void onUpdate(double deltaTime) {}

    void handleSdlEvent(SDL_Event* event);

protected:
    Window _window;
    Vulkan::Context _context;
    bool _done = false;

private:
    BuildInfo&& initialize(BuildInfo&& buildInfo);
};

extern std::unique_ptr<Application> CreateApplication();

} // namespace VoxelDynamics
