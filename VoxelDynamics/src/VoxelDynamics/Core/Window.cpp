#include "VoxelDynamics/Core/Window.hpp"

#include "VoxelDynamics/Core/Util.hpp"

namespace VoxelDynamics
{

Window::Window(BuildInfo buildInfo_)
    : buildInfo(std::move(buildInfo_))
    , _window(createWindow())
{
}

SDL_Window* Window::createWindow() const
{
    // configure window construction
    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (buildInfo.resizable)
        flags |= SDL_WINDOW_RESIZABLE;
    if (buildInfo.hidden)
        flags |= SDL_WINDOW_HIDDEN;

    // create the window
    SDL_Window* window = SDL_CreateWindow(
        buildInfo.title.c_str(),
        static_cast<int>(buildInfo.width),
        static_cast<int>(buildInfo.height),
        flags);
    if (!window)
        throw SDLException("Could not create window");

    // set the window's initial position
    std::visit(
        overloaded{
            [&](DefaultPlacement) {
                // do nothing
            },

            [&](CenteredPlacement) {
                // determine the current screen
                const SDL_DisplayID displayId = SDL_GetDisplayForWindow(window);
                if (!displayId)
                    throw SDLException("Could not find the current screen");

                // get dimensions of the screen
                const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayId);
                if (!mode)
                    throw SDLException("Could not determine the current screen's resolution");

                //  calculate the position of the top-left corner
                const uint32_t x = (mode->w - buildInfo.width) >> 1U;
                const uint32_t y = (mode->h - buildInfo.height) >> 1U;

                // apply the calculated position
                SDL_SetWindowPosition(window, static_cast<int>(x), static_cast<int>(y));
            },

            [&](FixedPlacement pos) {
                // apply the given position directly
                SDL_SetWindowPosition(window, static_cast<int>(pos.x), static_cast<int>(pos.y));
            },
        },
        buildInfo.placement);

    return window;
}

void Window::show() const
{
    if (!SDL_ShowWindow(_window))
        throw SDLException("Could not show window");
}

void Window::hide() const
{
    if (!SDL_HideWindow(_window))
        throw SDLException("Could not hide window");
}

std::pair<uint32_t, uint32_t> Window::getSize() const
{
    int width = 0, height = 0;
    if (!SDL_GetWindowSizeInPixels(_window, &width, &height))
        throw SDLException("Could not get window size");

    return std::make_tuple(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

} // namespace VoxelDynamics
