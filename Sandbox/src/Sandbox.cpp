// #include "GLFW/glfw3.h"

#include "glslang/Public/ResourceLimits.h"
#include "glslang/Public/ShaderLang.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "spirv-tools/optimizer.hpp"

// #include "taskflow/algorithm/for_each.hpp"
// #include "taskflow/taskflow.hpp"
// #include "vulkan/vulkan.hpp"

// GLFW ////////////////////////////////////////////////////////////////////////////////////////////

// namespace
// {

// GLFWwindow* initWindow(const std::string& windowTitle, uint32_t& outWidth, uint32_t& outHeight)
// {
//     glfwSetErrorCallback([](int error, const char* description) {
//         std::print("GLFW Error ({}): {}\n", error, description);
//     });

//     if (!glfwInit())
//         return nullptr;

//     const bool wantWholeArea = !(outWidth && outHeight);

//     glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//     glfwWindowHint(GLFW_RESIZABLE, wantWholeArea ? GLFW_FALSE : GLFW_TRUE);

//     GLFWmonitor* monitor    = glfwGetPrimaryMonitor();
//     const GLFWvidmode* mode = glfwGetVideoMode(monitor);

//     int x = 0;
//     int y = 0;
//     int w = mode->width;
//     int h = mode->height;

//     if (wantWholeArea)
//         glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);
//     else
//     {
//         w = static_cast<int>(outWidth);
//         h = static_cast<int>(outHeight);
//     }

//     GLFWwindow* window = glfwCreateWindow(w, h, windowTitle.c_str(), nullptr, nullptr);
//     if (!window)
//     {
//         glfwTerminate();
//         return nullptr;
//     }

//     if (wantWholeArea)
//         glfwSetWindowPos(window, x, y);

//     glfwGetWindowSize(window, &w, &h);

//     outWidth  = w;
//     outHeight = h;

//     glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int, int action, int) {
//         if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
//             glfwSetWindowShouldClose(window, GLFW_TRUE);
//     });

//     return window;
// }

// } // namespace

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

namespace
{

std::string readShaderFile(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
        throw std::runtime_error("Could not open shader file: " + filename);

    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

constexpr std::array<std::pair<std::string_view, EShLanguage>, 6> extension_map = {{
    {".vert", EShLangVertex},
    {".frag", EShLangFragment},
    {".geom", EShLangGeometry},
    {".comp", EShLangCompute},
    {".tesc", EShLangTessControl},
    {".tese", EShLangTessEvaluation},
}};

constexpr std::optional<EShLanguage> extensionToShaderStage(std::string_view extension) noexcept
{
    for (const auto& [ext, stage] : extension_map)
        if (ext == extension)
            return stage;
    return std::nullopt;
}

std::string_view getFileExtension(std::string_view filename) noexcept
{
    if (auto pos = filename.find_last_of('.'); pos != std::string_view::npos)
        return filename.substr(pos);
    return "";
}

EShLanguage shaderStageFromFilename(std::string_view filename)
{
    auto extension = getFileExtension(filename);
    if (auto stage = extensionToShaderStage(extension))
        return *stage;
    throw std::runtime_error("Unknown shader stage for file: " + std::string(filename));
}

void logShaderSource(const char* text)
{
    auto trim_cr = [](auto&& line_range) {
        std::string_view line{line_range};
        if (!line.empty() && line.back() == '\r')
            return line.substr(0, line.size() - 1);
        return line;
    };

    auto lines = std::string_view(text) | std::views::split('\n') | std::views::transform(trim_cr);

    for (const auto& [lineNum, line] : std::ranges::views::zip(std::ranges::views::iota(1), lines))
        VoxelDynamics::Log::Info("({:3d}) {}", lineNum, line);
}

std::vector<uint8_t> compileShader(
    EShLanguage stage,
    const std::string& code,
    const std::string& name,
    const TBuiltInResource* resources)
{
    std::unique_ptr<glslang::TShader> shader = std::make_unique<glslang::TShader>(stage);

    std::array<const char*, 1> raw_code{code.data()};
    std::array<const int, 1> raw_code_size{static_cast<int>(code.size())};
    std::array<const char*, 1> raw_code_name{name.data()};
    shader->setStringsWithLengthsAndNames(
        raw_code.data(), raw_code_size.data(), raw_code_name.data(), 1);
    shader->setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 330);
    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    // TODO implement GLSL includer
    auto includer = glslang::TShader::ForbidIncluder();

    std::string codePreproc;
    if (!shader->preprocess(
            resources, 330, ENoProfile, false, false, EShMsgDefault, &codePreproc, includer))
    {
        VoxelDynamics::Log::Warn("Shader preprocessing failed:");
        VoxelDynamics::Log::Warn("  {}", shader->getInfoLog());
        VoxelDynamics::Log::Warn("  {}", shader->getInfoDebugLog());
        logShaderSource(raw_code[0]);
        assert(false);
        return {};
    }

    auto messages = static_cast<EShMessages>(EShMsgDefault | EShMsgDebugInfo);
    if (!shader->parse(resources, 330, false, messages))
    {
        VoxelDynamics::Log::Warn("Shader parsing failed:");
        VoxelDynamics::Log::Warn("  {}", shader->getInfoLog());
        VoxelDynamics::Log::Warn("  {}", shader->getInfoDebugLog());
        logShaderSource(codePreproc.c_str());
        assert(false);
        return {};
    }

    std::unique_ptr<glslang::TProgram> program = std::make_unique<glslang::TProgram>();
    program->addShader(shader.get());

    if (!program->link(EShMsgDefault))
    {
        VoxelDynamics::Log::Warn("Shader linking failed:");
        VoxelDynamics::Log::Warn("  {}", program->getInfoLog());
        VoxelDynamics::Log::Warn("  {}", program->getInfoDebugLog());
        logShaderSource(codePreproc.c_str());
        assert(false);
        return {};
    }

    const glslang::TIntermediate* intermediate = program->getIntermediate(stage);

    glslang::SpvOptions options;
    options.generateDebugInfo                = true;
    options.stripDebugInfo                   = false;
    options.disableOptimizer                 = false;
    options.optimizeSize                     = true;
    options.disassemble                      = false;
    options.validate                         = true;
    options.emitNonSemanticShaderDebugInfo   = false;
    options.emitNonSemanticShaderDebugSource = false;

    std::vector<unsigned int> spirv;
    glslang::GlslangToSpv(*intermediate, spirv, &options);

    // optimize

    spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_3);
    optimizer.RegisterPass(spvtools::CreateStripNonSemanticInfoPass());
    optimizer.RegisterPass(spvtools::CreateRedundancyEliminationPass());

    std::vector<unsigned int> optimized_spirv;
    spv_optimizer_options optOptions = spvOptimizerOptionsCreate();
    spvOptimizerOptionsSetRunValidator(optOptions, false);
    optimizer.Run(spirv.data(), spirv.size(), &optimized_spirv, optOptions);

    // convert words to bytes

    auto byte_view = std::as_bytes(std::span{optimized_spirv});

    // NOLINTBEGIN
    std::vector<uint8_t> spirv_bytes(
        reinterpret_cast<const uint8_t*>(byte_view.data()),
        reinterpret_cast<const uint8_t*>(byte_view.data()) + byte_view.size());
    // NOLINTEND

    return spirv_bytes;
}

void saveSPIRVBinaryFile(const std::string& filename, const std::vector<uint8_t>& code)
{
    std::ofstream file(filename, std::ios::binary);

    if (file)
        file.write(
            reinterpret_cast<const char*>(code.data()), // NOLINT
            static_cast<std::streamsize>(code.size()));
}

void testShaderCompilation(const std::string& sourceFilename, const std::string& destFilename)
{
    std::string shaderSource = readShaderFile(sourceFilename);

    std::vector<uint8_t> spirv = compileShader(
        shaderStageFromFilename(sourceFilename),
        shaderSource,
        sourceFilename,
        GetDefaultResources());

    assert(!spirv.empty());

    saveSPIRVBinaryFile(destFilename, spirv);
}

} // namespace

int main()
{
    VoxelDynamics::Log::Init("Sandbox");

    glslang::InitializeProcess();

    try
    {
        testShaderCompilation("assets/main.vert", "main.vert.bin");
        testShaderCompilation("assets/main.frag", "main.frag.bin");
    }
    catch (const std::runtime_error& e)
    {
        std::println(stderr, "Caught an exception: {}", e.what());
    }

    glslang::FinalizeProcess();

    return 0;
}
