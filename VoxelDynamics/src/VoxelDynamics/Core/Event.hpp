#pragma once

#include "SDL3/SDL_keycode.h"

namespace VoxelDynamics::Event
{

// Window //////////////////////////////////////////////////////////////////////////////////////////

struct WindowClose
{
};

struct WindowResuze
{
    uint32_t width;
    uint32_t height;
};

// Keyboard ////////////////////////////////////////////////////////////////////////////////////////

struct KeyDown
{
    SDL_Keycode key;
    bool repeat;
};

} // namespace VoxelDynamics::Event
