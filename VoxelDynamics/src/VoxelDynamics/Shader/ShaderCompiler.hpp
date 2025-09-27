#pragma once

// adapted from
// https://github.com/KhronosGroup/glslang/blob/a57276bf558f5cf94d3a9854ebdf5a2236849a5a/StandAlone/StandAlone.cpp
// https://github.com/KhronosGroup/glslang/blob/a57276bf558f5cf94d3a9854ebdf5a2236849a5a/glslang/Public/ShaderLang.h
// https://github.com/corporateshark/lightweightvk/blob/79350790821c37174d9b8a94dfafb3c2840b5eb7/lvk/vulkan/VulkanUtils.cpp

#include "glslang/Public/ShaderLang.h"

namespace VoxelDynamics
{

class ShaderCompiler
{
public:
    enum class Options
    {
        /** Default is to give all required errors and extra warnings */
        None = EShMsgDefault,

        /** Be liberal in accepting input */
        RelaxedErrors = EShMsgRelaxedErrors,

        /** Suppress all warnings, except those required by the spec */
        SuppressWarnings = EShMsgSuppressWarnings,

        /** Print the AST intermediate representation */
        AST = EShMsgAST,

        /** Issue messages for SPIR-V generation */
        ValidateSpirV = EShMsgSpvRules,

        /** Issue messages for Vulkan-requirements of GLSL for SPIR-V */
        ValidateVulkan = EShMsgVulkanRules,

        /** Only print out errors produced by the preprocessor */
        PreprocessOnly = EShMsgOnlyPreprocessor,

        // /** Use HLSL parsing rules and semantics */
        // ReadHLSL = EShMsgReadHlsl,

        /** Get cascading errors; risks error-recovery issues, instead of an early exit */
        CascadingErrors = EShMsgCascadingErrors,

        /** For testing, don't eliminate uncalled functions */
        KeepUncalled = EShMsgKeepUncalled,

        // /** Allow block offsets to follow HLSL rules instead of GLSL rules */
        // HLSLOffsets = EShMsgHlslOffsets,

        /** Save debug information */
        DebugInfo = EShMsgDebugInfo,

        // /** Enable use of 16-bit types in SPIR-V for HLSL */
        // HLSL16BitTYpes = EShMsgHlslEnable16BitTypes,

        // /** Enable HLSL Legalization messages */
        // HLSLLegalization = EShMsgHlslLegalization,

        // /** Enable HLSL DX9 compatible mode (for samplers and semantics) */
        // HLSLDx9 = EShMsgHlslDX9Compatible,

        /** Print the builtin symbol table */
        SymbolTable = EShMsgBuiltinSymbolTable,

        /** Enhanced message readability */
        Enhanced = EShMsgEnhanced,

        /** Output Absolute path for messages */
        AbsolutePath = EShMsgAbsolutePath,

        /** Display error message column as well as line */
        DisplayColumn = EShMsgDisplayErrorColumn,

        /** Perform cross-stage optimizations during linking */
        OptimizeCrossStage = EShMsgLinkTimeOptimization,

        /** Validate shader inputs have matching outputs in previous stage */
        ValidateCrossStage = EShMsgValidateCrossStageIO,
    };

private:
    static EShMessages to_EShMessages(Options options) { return static_cast<EShMessages>(options); }

public:
    struct Source
    {
        EShLanguage stage;
        std::string text;
        std::optional<std::string> fileName         = std::nullopt;
        std::optional<std::string> binaryEntryPoint = std::nullopt;
        std::optional<std::string> sourceEntryPoint = std::nullopt;
    };

    struct SourceFile
    {
        std::string fileName;
        std::optional<EShLanguage> stage;
        std::optional<std::string> binaryEntryPoint = std::nullopt;
        std::optional<std::string> sourceEntryPoint = std::nullopt;
    };

    struct Parsed
    {
        EShLanguage stage;
        std::unique_ptr<glslang::TShader> shader;
        std::optional<std::string> fileName;
    };

    struct Linked
    {
        EShLanguage stage;
        std::vector<uint8_t> spirv;
        std::optional<std::string> fileName;
    };

    struct Stored
    {
        EShLanguage stage;
        std::string fileName;
    };

    static std::optional<Parsed> Parse(
        const Source& source,
        Options options,
        const std::vector<std::string>& includeDirs,
        const std::optional<std::string>& preamble);

    static std::optional<Parsed> ParseFile(
        const SourceFile& sourceFile,
        Options options,
        const std::vector<std::string>& includeDirs,
        const std::optional<std::string>& preamble);

    static std::optional<std::vector<Linked>> Link(
        const std::vector<Parsed>& shaders, Options options, bool debug = false);

    static std::optional<std::vector<Linked>> ParseAndLink(
        const std::vector<Source>& sources,
        Options options,
        const std::vector<std::string>& includeDirs,
        const std::optional<std::string>& preamble,
        bool debug = false);

    static std::optional<std::vector<Linked>> ParseAndLinkFiles(
        const std::vector<SourceFile>& sourceFiles,
        Options options,
        const std::vector<std::string>& includeDirs,
        const std::optional<std::string>& preamble,
        bool debug = false);

    static bool Store(const std::vector<Linked>& shaders);
    static std::optional<std::vector<Linked>> Load(const std::vector<Stored>& stored);

private:
    static void InfoIfNotEmpty(const char* str);
    static void WarnIfNotEmpty(const char* str);
    static void LogSource(const std::string& text);

    static std::string ReadFile(const std::string& fileName);
    static std::optional<EShLanguage> FileNameStage(const std::string_view fileName);

    static std::string LinkedFileName(
        EShLanguage stage, const std::optional<std::string>& fileName);
};

} // namespace VoxelDynamics

template <>
struct is_bitmask_enum<VoxelDynamics::ShaderCompiler::Options> : std::true_type
{
};
