#include "VoxelDynamics/Application.hpp"

#include "GLFW/glfw3.h"

namespace VoxelDynamics
{

Application::Application(const CreateInfo& createInfo)
{
    glfwSetErrorCallback([](int code, const char* description) {
        Log::Core::Error("GLFW Error ({}): {}", code, description);
    });

    if (glfwInit() != GLFW_TRUE)
        return;

    _window = std::make_unique<Window>(
        createInfo.title, std::make_pair(createInfo.width, createInfo.height));

    _context = std::make_unique<Vulkan::Context>(*_window, createInfo.context);
}

Application::~Application()
{
    _context.reset();
    _window.reset();
    glfwTerminate();
}

void Application::run() const
{
    while (_window->isAlive())
        _window->handleEvents();
}

} // namespace VoxelDynamics
