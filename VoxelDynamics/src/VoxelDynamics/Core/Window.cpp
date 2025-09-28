#include "VoxelDynamics/Core/Window.hpp"

#include "VoxelDynamics/Core/Log.hpp"

namespace VoxelDynamics
{

Window::Window(const std::string& title, std::optional<std::pair<uint32_t, uint32_t>> size)
{
    glfwSetErrorCallback([](int code, const char* description) {
        Log::Core::Error("GLFW Error ({}): {}", code, description);
    });

    if (glfwInit() != GLFW_TRUE)
        return;

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

Window::~Window()
{
    glfwTerminate();
}

std::pair<uint32_t, uint32_t> Window::getSize() const
{
    int width{}, height{};
    glfwGetWindowSize(_window, &width, &height);
    return std::make_pair(width, height);
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
