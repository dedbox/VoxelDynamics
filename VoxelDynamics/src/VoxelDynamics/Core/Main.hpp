#pragma once

#define SDL_MAIN_USE_CALLBACKS 1

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_main.h"

#include "VoxelDynamics/Core/Application.hpp"
#include "VoxelDynamics/Core/Time.hpp"

using namespace VoxelDynamics;

inline SDL_AppResult SDL_AppInit(void** appstate, int /*argc*/, char** /*argv*/)
{
    try
    {
        std::unique_ptr<Application> app = CreateApplication();
        app->onCreate();
        app->onCreated();

        *appstate = app.release();

        return SDL_APP_CONTINUE;
    }
    catch (const SDLException& exn)
    {
        Log::Core::Critical("SDL Error: {}", exn.what());
        return SDL_APP_FAILURE;
    }
}

inline SDL_AppResult SDL_AppIterate(void* appstate)
{
    static double lastFrameTime = Time::Seconds() - 0.016;
    static double thisFrameTime = Time::Seconds();

    const double deltaTime = thisFrameTime - lastFrameTime;
    lastFrameTime          = thisFrameTime;

    try
    {
        auto app = static_cast<Application*>(appstate);

        if (app->isDone())
            return SDL_APP_SUCCESS;

        app->onUpdate(deltaTime);

        return SDL_APP_CONTINUE;
    }
    catch (const SDLException& exn)
    {
        Log::Core::Critical("SDL Error: {}", exn.what());
        return SDL_APP_FAILURE;
    }
}

inline SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    try
    {
        auto app = static_cast<Application*>(appstate);

        app->handleSdlEvent(event);

        return SDL_APP_CONTINUE;
    }
    catch (const SDLException& exn)
    {
        Log::Core::Critical("SDL Error: {}", exn.what());
        return SDL_APP_FAILURE;
    }
}

inline void SDL_AppQuit(void* appstate, SDL_AppResult /*result*/)
{
    delete static_cast<VoxelDynamics::Application*>(appstate); // NOLINT
}
