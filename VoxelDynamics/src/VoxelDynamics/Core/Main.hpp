#pragma once

#define SDL_MAIN_USE_CALLBACKS 1

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_main.h"

#include "VoxelDynamics/Core/Application.hpp"

inline SDL_AppResult SDL_AppInit(void** appstate, int /*argc*/, char** /*argv*/)
{
    try
    {
        std::unique_ptr<VoxelDynamics::Application> app = VoxelDynamics::CreateApplication();

        *appstate = app.release();

        return SDL_APP_CONTINUE;
    }
    catch (const SDLException& exn)
    {
        VoxelDynamics::Log::Core::Critical("SDL Error: {}", exn.what());
        return SDL_APP_FAILURE;
    }
}

inline SDL_AppResult SDL_AppIterate(void* appstate)
{
    try
    {
        auto app = static_cast<VoxelDynamics::Application*>(appstate);

        if (app->isDone())
            return SDL_APP_SUCCESS;

        app->update();

        return SDL_APP_CONTINUE;
    }
    catch (const SDLException& exn)
    {
        VoxelDynamics::Log::Core::Critical("SDL Error: {}", exn.what());
        return SDL_APP_FAILURE;
    }
}

inline SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    try
    {
        auto app = static_cast<VoxelDynamics::Application*>(appstate);

        app->handleSdlEvent(event);

        return SDL_APP_CONTINUE;
    }
    catch (const SDLException& exn)
    {
        VoxelDynamics::Log::Core::Critical("SDL Error: {}", exn.what());
        return SDL_APP_FAILURE;
    }
}

inline void SDL_AppQuit(void* appstate, SDL_AppResult /*result*/)
{
    delete static_cast<VoxelDynamics::Application*>(appstate); // NOLINT
}
