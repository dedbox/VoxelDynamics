#include "VoxelDynamics/Core/Window.hpp"

#include "vulkan/vulkan_core.h"

namespace VoxelDynamics
{

Window::Window(const std::string& title, std::optional<std::pair<uint32_t, uint32_t>> size)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, size ? GLFW_TRUE : GLFW_FALSE);

    GLFWmonitor* monitor    = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    int x = 0;
    int y = 0;
    int w = mode->width;
    int h = mode->height;

    if (!size)
        glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);
    else
    {
        w = static_cast<int>(size->first);
        h = static_cast<int>(size->second);
    }

    _window = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
    if (!_window)
        return;

    if (!size)
        glfwSetWindowPos(_window, x, y);

    glfwSetKeyCallback(_window, [](GLFWwindow* window, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    });
}

std::pair<uint32_t, uint32_t> Window::getSize() const
{
    int width{}, height{};
    glfwGetWindowSize(_window, &width, &height);
    return std::make_pair(width, height);
}

VkSurfaceKHR Window::createSurface(const VkInstance& instance) const
{
    VkSurfaceKHR surface{VK_NULL_HANDLE};
    VkResult error = glfwCreateWindowSurface(instance, _window, nullptr, &surface);
    Log::Core::Assert(!error, "Could not create window surface");
    return surface;
}

bool Window::isAlive() const
{
    return !glfwWindowShouldClose(_window);
}

void Window::handleEvents() const
{
    glfwPollEvents();
}

} // namespace VoxelDynamics
