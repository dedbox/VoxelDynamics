// #include "taskflow/algorithm/for_each.hpp"
// #include "taskflow/taskflow.hpp"

// #include "stb_image.h"
// #include "stb_image_resize2.h"

// #include "ktx.h"

#include <VoxelDynamics.hpp>

int main()
{
    using namespace VoxelDynamics;

    Log::Init("Sandbox");

    uint32_t width  = 960;
    uint32_t height = 540;

    Window window("Simple Example", std::make_pair(width, height));

    VulkanContext context(
        window,
        {.appName             = "Simple Example",
         .appVersion          = Version(1, 0, 0),
         .width               = width,
         .height              = height,
         .preferredDeviceType = DeviceType::Discrete});

    while (window.isAlive())
        window.handleEvents();

    return 0;
}

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

// int main()
// {
//     VoxelDynamics::Log::Init("Sandbox");

//     glslang::InitializeProcess();

//     std::vector<VoxelDynamics::ShaderCompiler::SourceFile> sourceFiles{
//         {.fileName = "assets/main.vert"},
//         {.fileName = "assets/main.frag"},
//     };

//     VoxelDynamics::ShaderCompiler::Options options{
//         VoxelDynamics::ShaderCompiler::Options::ValidateSpirV |
//         VoxelDynamics::ShaderCompiler::Options::ValidateVulkan |
//         VoxelDynamics::ShaderCompiler::Options::DebugInfo};

//     const auto shaders = VoxelDynamics::ShaderCompiler::ParseAndLinkFiles(
//         sourceFiles, options, {}, std::nullopt, true);

//     if (!shaders)
//     {
//         std::println("Shader compilation failed.");
//         return -1;
//     }

//     VoxelDynamics::ShaderCompiler::Store(*shaders);

//     glslang::FinalizeProcess();

//     return 0;
// }

// BC7 /////////////////////////////////////////////////////////////////////////////////////////////

// namespace
// {

// // from
// //
// https://github.com/corporateshark/lightweightvk/blob/92219fb90b9f2b66bfc13708e60cee5b3fbf7e74/lvk/LVK.h
// constexpr uint32_t calcNumMipLevels(uint32_t width, uint32_t height)
// {
//     uint32_t levels = 1;

//     while ((width | height) >> levels)
//         levels++;

//     return levels;
// }

// } // namespace

// int main()
// {
//     using Log = VoxelDynamics::Log;
//     Log::Init("Sandbox");

//     const std::string inFileName  = "assets/wood.jpg";
//     const std::string outFileName = "assets/wood.ktx";

//     Log::Info("Loading texture from file `{}'", inFileName);
//     const int numChannels = 4;
//     int origW = 0, origH = 0;
//     uint8_t* pixels = stbi_load(inFileName.c_str(), &origW, &origH, nullptr, numChannels);

//     Log::Assert(pixels, "Could not load texture `{}'", inFileName);

//     Log::Info("Creating KTX2 texture");
//     const uint32_t numMipLevels = calcNumMipLevels(origW, origH);

//     ktxTextureCreateInfo createInfoKTX2{
//         .vkFormat        = VK_FORMAT_R8G8B8A8_UNORM,
//         .baseWidth       = static_cast<uint32_t>(origW),
//         .baseHeight      = static_cast<uint32_t>(origH),
//         .baseDepth       = 1U,
//         .numDimensions   = 2U,
//         .numLevels       = numMipLevels,
//         .numLayers       = 1U,
//         .numFaces        = 1U,
//         .isArray         = KTX_FALSE,
//         .generateMipmaps = KTX_FALSE,
//     };

//     ktxTexture2* textureKTX2 = nullptr;
//     if (ktxTexture2_Create(&createInfoKTX2, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &textureKTX2) !=
//         KTX_SUCCESS)
//         Log::Assert(false, "Could not create KTX2 texture");

//     int w = origW;
//     int h = origH;

//     Log::Info("Generating custom mip-pyramid");
//     for (uint32_t i = 0; i != numMipLevels; ++i)
//     {
//         size_t offset = 0;
//         ktxTexture2_GetImageOffset(textureKTX2, i, 0, 0, &offset);
//         stbir_resize_uint8_linear(
//             pixels, origW, origH, 0, textureKTX2->pData + offset, w, h, 0, STBIR_RGBA); // NOLINT

//         h = h > 1 ? h >> 1 : 1; // NOLINT
//         w = w > 1 ? w >> 1 : 1; // NOLINT
//     }

//     Log::Info("Compressing KTX2 texture to Basis");
//     if (ktxTexture2_CompressBasis(textureKTX2, 255) != KTX_SUCCESS)
//         Log::Assert(false, "Could not compress KTX2 texture");

//     Log::Info("Transcoding KTX2 texture");
//     if (ktxTexture2_TranscodeBasis(textureKTX2, KTX_TTF_BC7_RGBA, 0) != KTX_SUCCESS)
//         Log::Assert(false, "Could not transcode KTX2 texture");

//     Log::Info("Writing KTX2 texture to file `{}'", outFileName);
//     ktxTexture2_WriteToNamedFile(textureKTX2, outFileName.c_str());
//     ktxTexture2_Destroy(textureKTX2);

//     if (pixels)
//         stbi_image_free(pixels);

//     return 0;
// }
