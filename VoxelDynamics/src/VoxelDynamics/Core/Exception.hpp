#pragma once

#include "SDL3/SDL_error.h"

class SDLException : public std::runtime_error
{
public:
    explicit SDLException(const std::string& message)
        : std::runtime_error(std::format("{}: {}", message, SDL_GetError()))
    {
    }
};
