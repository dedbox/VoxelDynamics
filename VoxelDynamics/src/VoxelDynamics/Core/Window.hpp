#pragma once

#include "SDL3/SDL_video.h"

namespace VoxelDynamics
{

class Window
{
public:
    // default placement
    struct DefaultPlacement
    {
    };

    struct CenteredPlacement
    {
    };

    struct FixedPlacement
    {
        uint32_t x = 0;
        uint32_t y = 0;
    };

    using Placement = std::variant<DefaultPlacement, CenteredPlacement, FixedPlacement>;

    // custruction
    const struct BuildInfo
    {
        std::string title   = "VoxelDynamics Application";
        uint32_t width      = 800;
        uint32_t height     = 600;
        Placement placement = DefaultPlacement();
        bool resizable      = false;
        bool hidden         = false;
    } buildInfo;

    explicit Window(BuildInfo buildInfo);

    // destruction
    ~Window() = default;

    // prevent move
    Window(Window&&) noexcept            = delete;
    Window& operator=(Window&&) noexcept = delete;

    // prevent copy
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    // proxy dereference operator
    SDL_Window* operator*() { return _window; }
    const SDL_Window* operator*() const { return _window; }

    void show() const;
    void hide() const;

    std::pair<uint32_t, uint32_t> getSize() const;

private:
    SDL_Window* _window;

    SDL_Window* createWindow() const;
};

} // namespace VoxelDynamics
