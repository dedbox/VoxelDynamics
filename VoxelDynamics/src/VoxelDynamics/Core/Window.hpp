#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

namespace VoxelDynamics
{

class Window
{
public:
    explicit Window(
        const std::string& title, std::optional<std::pair<uint32_t, uint32_t>> size = std::nullopt);

    ~Window();

    // allow copy
    Window(const Window&)            = default;
    Window& operator=(const Window&) = default;

    // prevent move
    Window(Window&&)            = delete;
    Window& operator=(Window&&) = delete;

    std::pair<uint32_t, uint32_t> getSize() const;

    VkSurfaceKHR createSurface(const VkInstance& instance) const;

    bool isAlive() const;
    void handleEvents() const;

private:
    GLFWwindow* _window{};
};

} // namespace VoxelDynamics
