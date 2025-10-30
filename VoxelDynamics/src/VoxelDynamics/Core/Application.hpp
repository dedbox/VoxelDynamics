#pragma once

#include "SDL3/SDL_events.h"

#include "VoxelDynamics/Core/Window.hpp"
#include "VoxelDynamics/Vulkan/Context.hpp"
#include "VoxelDynamics/Vulkan/Renderer.hpp"

namespace VoxelDynamics
{

/** The top-level controller and entry point for the graphics engine.
 *
 * The Application class encapsulates the core components needed for rendering and drives the main
 * application loop. It is responsible for the high-level management of the engine, including the
 * construction of the platform-native window, the graphics context, and the renderer. It also
 * handles events and performs each iteration of the main update-render loop.
 */
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
        Vulkan::Renderer::BuildInfo renderer{};
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
    virtual void onCreate() {}
    void onCreated() { _renderer.resetOneShotBuffers(); }

    virtual void onUpdate(double deltaTime) {}

    void handleSdlEvent(SDL_Event* event);

    void quit() { _done = true; }
    bool isDone() const { return _done; }
    void onQuit() const { _context.getDevice()->waitIdle(); }

protected:
    Window _window;
    Vulkan::Context _context;
    Vulkan::Renderer _renderer;

    bool _done = false;

private:
    BuildInfo&& initialize(BuildInfo&& buildInfo);
};

extern std::unique_ptr<Application> CreateApplication();

} // namespace VoxelDynamics
