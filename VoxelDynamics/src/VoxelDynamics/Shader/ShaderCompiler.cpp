#include "VoxelDynamics/Shader/ShaderCompiler.hpp"

#include "glslang/Public/ResourceLimits.h"
#include "glslang/SPIRV/GlslangToSpv.h"

#include "VoxelDynamics/Shader/DirStackFileIncluder.hpp"

namespace VoxelDynamics
{

std::optional<ShaderCompiler::Parsed> ShaderCompiler::Parse(
    const Source& source,
    Options options,
    const std::vector<std::string>& includeDirs,
    const std::optional<std::string>& preamble)
{
    auto shader = std::make_unique<glslang::TShader>(source.stage);

    const std::array<const char*, 1> text{source.text.c_str()};
    const std::array<const int, 1> text_size{static_cast<int>(source.text.size())};
    if (source.fileName)
    {
        const std::array<const char*, 1> name{source.fileName->c_str()};
        shader->setStringsWithLengthsAndNames(text.data(), text_size.data(), name.data(), 1);
    }
    else
        shader->setStringsWithLengths(text.data(), text_size.data(), 1);

    if (source.binaryEntryPoint)
        shader->setEntryPoint(source.binaryEntryPoint->c_str());

    if (source.sourceEntryPoint)
    {
        if (!source.binaryEntryPoint)
            Log::Core::Warn(
                "Changing source entry point name without setting a binary entry point name");
        shader->setSourceEntryPoint(source.sourceEntryPoint->c_str());
    }

    if (preamble)
        shader->setPreamble(preamble->c_str());

    shader->setEnvInput(glslang::EShSourceGlsl, source.stage, glslang::EShClientVulkan, 450);
    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    EShMessages messages = to_EShMessages(options);

    DirStackFileIncluder includer;
    for (const auto& dir : includeDirs)
        includer.pushExternalLocalDirectory(dir);

    if (!shader->parse(GetDefaultResources(), 450, false, messages, includer))
    {
        Log::Core::Warn("Shader parsing failed");
        WarnIfNotEmpty(shader->getInfoLog());
        WarnIfNotEmpty(shader->getInfoDebugLog());
        LogSource(source.text);
        return std::nullopt;
    }

    InfoIfNotEmpty(source.fileName.value_or("").c_str());
    InfoIfNotEmpty(shader->getInfoLog());
    InfoIfNotEmpty(shader->getInfoDebugLog());

    return Parsed(source.stage, std::move(shader), source.fileName);
}

std::optional<ShaderCompiler::Parsed> ShaderCompiler::ParseFile(
    const SourceFile& sourceFile,
    Options options,
    const std::vector<std::string>& includeDirs,
    const std::optional<std::string>& preamble)
{
    auto stage = FileNameStage(sourceFile.fileName);
    if (!stage)
        return std::nullopt;

    Source source{
        .stage            = sourceFile.stage.value_or(*stage),
        .text             = ReadFile(sourceFile.fileName),
        .fileName         = sourceFile.fileName,
        .binaryEntryPoint = sourceFile.binaryEntryPoint,
        .sourceEntryPoint = sourceFile.sourceEntryPoint};

    return Parse(source, options, includeDirs, preamble);
}

std::optional<std::vector<ShaderCompiler::Linked>> ShaderCompiler::Link(
    const std::vector<Parsed>& shaders, Options options, bool debug)
{
    auto program = std::make_unique<glslang::TProgram>();
    std::map<EShLanguage, std::string> fileNames;
    for (const auto& parsed : shaders)
    {
        program->addShader(parsed.shader.get());
        fileNames.emplace(parsed.stage, std::move(parsed.fileName).value());
    }

    if (!program->link(to_EShMessages(options)))
    {
        Log::Core::Warn("Shader linking failed");
        WarnIfNotEmpty(program->getInfoLog());
        WarnIfNotEmpty(program->getInfoDebugLog());
        return std::nullopt;
    }

    InfoIfNotEmpty(program->getInfoLog());
    InfoIfNotEmpty(program->getInfoDebugLog());

    std::vector<std::tuple<EShLanguage, glslang::TIntermediate*, std::string>> intermediates;
    for (int stageNum = 0; stageNum < EShLangCount; ++stageNum)
    {
        auto stage = static_cast<EShLanguage>(stageNum);
        if (auto* intermediate = program->getIntermediate(stage))
            intermediates.emplace_back(stage, intermediate, fileNames[stage]);
    }

    spv::SpvBuildLogger logger;

    glslang::SpvOptions spvOptions;
    spvOptions.compileOnly                      = false;
    spvOptions.disableOptimizer                 = false;
    spvOptions.optimizeSize                     = true;
    spvOptions.disassemble                      = false;
    spvOptions.emitNonSemanticShaderDebugInfo   = false;
    spvOptions.emitNonSemanticShaderDebugSource = false;
    if (debug)
    {
        spvOptions.generateDebugInfo = true;
        spvOptions.stripDebugInfo    = false;
        spvOptions.validate          = true;
    }
    else
    {
        spvOptions.generateDebugInfo = false;
        spvOptions.stripDebugInfo    = true;
        spvOptions.validate          = false;
    }

    std::vector<Linked> linked;
    for (const auto& [stage, intermediate, fileName] : intermediates)
    {
        std::vector<unsigned int> spirv;
        glslang::GlslangToSpv(*intermediate, spirv, &logger, &spvOptions);
        InfoIfNotEmpty(logger.getAllMessages().c_str());

        // convert words to bytes

        auto byte_view = std::as_bytes(std::span(spirv));

        // NOLINTBEGIN
        std::vector<uint8_t> bytes(
            reinterpret_cast<const uint8_t*>(byte_view.data()),
            reinterpret_cast<const uint8_t*>(byte_view.data()) + byte_view.size());
        // NOLINTEND

        linked.emplace_back(stage, bytes, fileName);
    }

    return linked;
}

std::optional<std::vector<ShaderCompiler::Linked>> ShaderCompiler::ParseAndLink(
    const std::vector<Source>& sources,
    Options options,
    const std::vector<std::string>& includeDirs,
    const std::optional<std::string>& preamble,
    bool debug)
{
    std::vector<Parsed> shaders;
    shaders.reserve(sources.size());

    for (const auto& source : sources)
    {
        auto parsed = Parse(source, options, includeDirs, preamble);
        if (!parsed)
            return std::nullopt;
        shaders.push_back(std::move(parsed).value());
    }

    return Link(shaders, options, debug);
}

std::optional<std::vector<ShaderCompiler::Linked>> ShaderCompiler::ParseAndLinkFiles(
    const std::vector<SourceFile>& sourceFiles,
    Options options,
    const std::vector<std::string>& includeDirs,
    const std::optional<std::string>& preamble,
    bool debug)
{
    std::vector<Parsed> shaders;
    shaders.reserve(sourceFiles.size());

    for (const auto& sourceFile : sourceFiles)
    {
        auto parsed = ParseFile(sourceFile, options, includeDirs, preamble);
        if (!parsed)
            return std::nullopt;
        shaders.push_back(std::move(parsed).value());
    }

    return Link(shaders, options, debug);
}

bool ShaderCompiler::Store(const std::vector<Linked>& shaders)
{
    for (const auto& shader : shaders)
    {
        const std::string fileName = LinkedFileName(shader.stage, shader.fileName);

        std::ofstream file(fileName, std::ios::binary);
        if (!file)
        {
            Log::Core::Error("Could not open file `{}' for writing", fileName);
            return false;
        }

        file.write(
            reinterpret_cast<const char*>(shader.spirv.data()), // NOLINT
            static_cast<std::streamsize>(shader.spirv.size()));
    }
    return true;
}

std::optional<std::vector<ShaderCompiler::Linked>> ShaderCompiler::Load(
    const std::vector<Stored>& stored)
{
    std::vector<Linked> linked;
    for (const auto& shader : stored)
    {
        std::ifstream file(shader.fileName, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            Log::Core::Error("Could not open file `{}' for reading", shader.fileName);
            return std::nullopt;
        }

        const auto fileSize = file.tellg();
        if (fileSize < 0)
        {
            Log::Core::Error("Could not determine size of file `{}'", shader.fileName);
            return std::nullopt;
        }

        file.seekg(0);

        std::vector<uint8_t> spirv(static_cast<size_t>(fileSize));
        file.read(reinterpret_cast<char*>(spirv.data()), fileSize); // NOLINT

        if (!file)
        {
            Log::Core::Error("Error reading file `{}'", shader.fileName);
            return std::nullopt;
        }

        linked.emplace_back(shader.stage, std::move(spirv), shader.fileName);
    }

    return linked;
}

void ShaderCompiler::InfoIfNotEmpty(const char* str)
{
    if (str && *str)
        Log::Core::Info("{}", str);
}

void ShaderCompiler::WarnIfNotEmpty(const char* str)
{
    if (str && *str)
        Log::Core::Warn("{}", str);
}

void ShaderCompiler::LogSource(const std::string& text)
{
    auto trim_cr = [](auto&& line_range) {
        std::string_view line{line_range};
        if (!line.empty() && line.back() == '\r')
            return line.substr(0, line.size() - 1);
        return line;
    };

    auto lines = text | std::ranges::views::split('\n') | std::ranges::views::transform(trim_cr);

    for (const auto& [lineNum, line] : std::ranges::views::zip(std::ranges::views::iota(1), lines))
        Log::Core::Info("({:3d}) {}", lineNum, line);
}

std::string ShaderCompiler::ReadFile(const std::string& fileName)
{
    std::ifstream file{fileName};
    std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    return text;
}

namespace
{

constexpr std::array<std::pair<std::string_view, EShLanguage>, 6> extension_map = {{
    {".vert", EShLangVertex},
    {".frag", EShLangFragment},
    {".geom", EShLangGeometry},
    {".comp", EShLangCompute},
    {".tesc", EShLangTessControl},
    {".tese", EShLangTessEvaluation},
}};

constexpr std::optional<EShLanguage> FileExtensionStage(std::string_view extension) noexcept
{
    for (const auto& [ext, stage] : extension_map)
        if (ext == extension)
            return stage;
    return std::nullopt;
}

std::optional<std::string_view> FileExtension(std::string_view fileName) noexcept
{
    if (auto pos = fileName.find_last_of('.'); pos != std::string_view::npos)
        return fileName.substr(pos);
    return std::nullopt;
}

} // namespace

std::optional<EShLanguage> ShaderCompiler::FileNameStage(std::string_view fileName)
{
    auto extension = FileExtension(fileName);
    if (!extension)
    {
        Log::Core::Error("Filename `{}' does not have a file extension", fileName);
        return std::nullopt;
    }

    auto stage = FileExtensionStage(*extension);
    if (!stage)
        Log::Core::Error("Unknown file extension for shader file `{}'", fileName);

    return stage;
}

std::string ShaderCompiler::LinkedFileName(
    EShLanguage stage, const std::optional<std::string>& fileName)
{
    if (!fileName)
    {
        // clang-format off
        switch (stage) {
        case EShLangVertex:          return "vert.spv";
        case EShLangTessControl:     return "tesc.spv";
        case EShLangTessEvaluation:  return "tese.spv";
        case EShLangGeometry:        return "geom.spv";
        case EShLangFragment:        return "frag.spv";
        case EShLangCompute:         return "comp.spv";
        case EShLangRayGen:          return "rgen.spv";
        case EShLangIntersect:       return "rint.spv";
        case EShLangAnyHit:          return "rahit.spv";
        case EShLangClosestHit:      return "rchit.spv";
        case EShLangMiss:            return "rmiss.spv";
        case EShLangCallable:        return "rcall.spv";
        case EShLangMesh :           return "mesh.spv";
        case EShLangTask :           return "task.spv";
        default:                     return "unknown"; 
        }
        // clang-format on
    }
    return *fileName + ".spv";
}

} // namespace VoxelDynamics
