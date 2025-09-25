#include "GLFW/glfw3.h"

#include <VoxelDynamics/Hello.hpp>

namespace
{

GLFWwindow* initWindow(const std::string& windowTitle, uint32_t& outWidth, uint32_t& outHeight)
{
    glfwSetErrorCallback([](int error, const char* description) {
        std::print("GLFW Error ({}): {}\n", error, description);
    });

    if (!glfwInit())
        return nullptr;

    const bool wantWholeArea = !(outWidth && outHeight);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, wantWholeArea ? GLFW_FALSE : GLFW_TRUE);

    GLFWmonitor* monitor    = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    int x = 0;
    int y = 0;
    int w = mode->width;
    int h = mode->height;

    if (wantWholeArea)
        glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);
    else
    {
        w = static_cast<int>(outWidth);
        h = static_cast<int>(outHeight);
    }

    GLFWwindow* window = glfwCreateWindow(w, h, windowTitle.c_str(), nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return nullptr;
    }

    if (wantWholeArea)
        glfwSetWindowPos(window, x, y);

    glfwGetWindowSize(window, &w, &h);

    outWidth  = w;
    outHeight = h;

    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    });

    return window;
}

} // namespace

int main()
{
    uint32_t width  = 1280;
    uint32_t height = 800;

    GLFWwindow* window = initWindow("GLFW Example", width, height);

    VoxelDynamics::Hello();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
