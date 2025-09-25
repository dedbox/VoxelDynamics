#include "GLFW/glfw3.h"

// #include "taskflow/algorithm/for_each.hpp"
// #include "taskflow/taskflow.hpp"

#include "vulkan/vulkan.hpp"

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

// GLFW ////////////////////////////////////////////////////////////////////////////////////////////

// int main()
// {
//     uint32_t width  = 1280;
//     uint32_t height = 800;

//     GLFWwindow* window = initWindow("GLFW Example", width, height);

//     VoxelDynamics::Hello();

//     while (!glfwWindowShouldClose(window))
//     {
//         glfwPollEvents();
//     }

//     glfwTerminate();
//     return 0;
// }

// Taskflow ////////////////////////////////////////////////////////////////////////////////////////

// int main()
// {
//     tf::Taskflow taskflow;

//     auto task =
//         taskflow.for_each_index(1, 9, 1, [](int i) { std::print("{}", i);
//         }).name("for_each_index");

//     taskflow.emplace([]() { std::println("\nS - Start"); }).name("S").precede(task);
//     taskflow.emplace([]() { std::println("\nT - End"); }).name("T").succeed(task);

//     std::ofstream os("taskflow.dot");
//     taskflow.dump(os);

//     tf::Executor executor;
//     executor.run(taskflow).wait();

//     return 0;
// }

// glslang /////////////////////////////////////////////////////////////////////////////////////////

// namespace
// {

// std::vector<uint8_t> compileShader(
//     EShLanguage stage, const char* code, const TBuiltInResource* resources)
// {
//     std::unique_ptr<glslang::TShader> shader = std::make_unique<glslang::TShader>(stage);

//     shader->setStrings(&code, 1);
//     shader->setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
//     shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
//     shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

//     auto includer = glslang::TShader::ForbidIncluder();

//     std::string output;
//     if (!shader->preprocess(
//             resources, 100, ENoProfile, false, false, EShMsgDefault, &output, includer))
//     {
//         // ...
//     }
// }

// } // namespace

int main()
{
    VoxelDynamics::Log::Init("Sandbox");

    VoxelDynamics::Log::Trace("Hello {}", "world");
    VoxelDynamics::Log::Warn("Goodbye {}", "moon");

    VoxelDynamics::Core::Assert(1 + 1 == 2, "{} + {} = {}", 1, 1, 2);
    VoxelDynamics::Assert(2 + 2 == 4, "{} + {} = {}", 2, 2, 4);

    VoxelDynamics::Core::Assert(1 + 1 == 3, "{} + {} != {}", 1, 1, 3);

    std::println("done");
}
