#include "VoxelDynamics/Renderer/Vulkan/Context.hpp"

#include "SDL3/SDL_vulkan.h"

#include "VoxelDynamics/Core/Util.hpp"

namespace VoxelDynamics::Vulkan
{

Context::Context(SDL_Window* window, const BuildInfo& contextInfo)
    : _instance(createInstance(contextInfo))
    , _surface(createSurface(window))
    , _physicalDevice(pickPhysicalDevice(contextInfo.preferredDeviceType))
    , _device(createDevice())
    , _swapChain(createSwapChain(window))
    , _pipeline(createPipeline())
{
}

// Instance ////////////////////////////////////////////////////////////////////////////////////////

vk::raii::Instance Context::createInstance(const BuildInfo& contextInfo) const
{
    const vk::ApplicationInfo appInfo(
        contextInfo.appName.c_str(),
        contextInfo.appVersion,
        EngineName.c_str(),
        EngineVersion,
        vk::ApiVersion13);

    const auto layers     = InstanceLayers();
    const auto extensions = InstanceExtensions();

    CheckInstanceLayers(layers);
    CheckInstanceExtensions(extensions);

    const auto instanceCreateInfo = vk::InstanceCreateInfo({}, &appInfo, layers, extensions);
    if constexpr (is_debugging_enabled)
    {
        const auto debugMessengerCreateInfo = DebugUtilsMessengerCreateInfoEXT();
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
            createInfoChain(instanceCreateInfo, debugMessengerCreateInfo);
        return vk::raii::Instance(_context, createInfoChain.get<vk::InstanceCreateInfo>());
    }
    else
        return vk::raii::Instance(_context, instanceCreateInfo);
}

constexpr std::vector<const char*> Context::InstanceLayers()
{
    if constexpr (is_debugging_enabled)
        return {
            // "VK_LAYER_LUNARG_api_dump",
            "VK_LAYER_KHRONOS_validation",
        };
    else
        return {};
}

std::vector<const char*> Context::InstanceExtensions()
{
    uint32_t count      = 0;
    auto raw_extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    std::vector<const char*> extensions(raw_extensions, raw_extensions + count);
    if (is_debugging_enabled)
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    return extensions;
}

void Context::CheckInstanceLayers(const std::vector<const char*>& layers)
{
    auto required =
        layers | std::ranges::views::transform([](const auto layer) { return std::string(layer); });

    for (const auto& layer : required)
        Log::Core::Trace("Required validation layer: {}", layer);

    auto available =
        vk::enumerateInstanceLayerProperties() |
        std::ranges::views::transform([](const auto p) { return std::string(p.layerName); });

    for (const auto& layer : available)
        Log::Core::Trace("Available validation layer: {}", layer);

    std::set<std::string> missing(required.begin(), required.end());
    for (const auto& layer : available)
        missing.erase(layer);

    if (missing.empty())
        for (const auto& layer : layers)
            Log::Core::Info("Enabling validation layer: {}", layer);
    else
        for (const auto& layer : missing)
            Log::Core::Warn("Unsupported validation layer: {}", layer);
}

void Context::CheckInstanceExtensions(const std::vector<const char*>& extensions)
{
    const auto required =
        extensions | std::ranges::views::transform([](const auto ext) { return std::string(ext); });

    for (const auto& ext : required)
        Log::Core::Trace("Required instance extension: {}", ext);

    const auto available =
        vk::enumerateInstanceExtensionProperties() |
        std::ranges::views::transform([](const auto p) { return std::string(p.extensionName); });

    for (const auto& ext : available)
        Log::Core::Trace("Available instance extension: {}", ext);

    std::set<std::string> missing(required.begin(), required.end());
    for (const auto& layer : available)
        missing.erase(layer);

    if (missing.empty())
        for (const auto& ext : extensions)
            Log::Core::Info("Enabling instance extension: {}", ext);
    else
        for (const auto& ext : missing)
            Log::Core::Warn("Unsupported instance extension: {}", ext);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Context::DebugUtilsMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
    vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* /*pUserData*/)
{
    switch (messageSeverity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        Log::Core::Trace("{}", pCallbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        Log::Core::Info("{}", pCallbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        Log::Core::Warn("{}", pCallbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        Log::Core::Error("{}", pCallbackData->pMessage);
        break;
    }
    return vk::False;
}

constexpr vk::DebugUtilsMessengerCreateInfoEXT Context::DebugUtilsMessengerCreateInfoEXT()
{
    constexpr vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    constexpr vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    constexpr vk::DebugUtilsMessengerCreateInfoEXT createInfo(
        {}, severityFlags, messageTypeFlags, &DebugUtilsMessengerCallback);

    return createInfo;
}

// Surface /////////////////////////////////////////////////////////////////////////////////////////

vk::raii::SurfaceKHR Context::createSurface(SDL_Window* window) const
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window, *_instance, nullptr, &surface))
        throw SDLException("Could not create Vulkan surface");
    return vk::raii::SurfaceKHR(_instance, surface);
}

// Physical Device /////////////////////////////////////////////////////////////////////////////////

Context::PhysicalDevice Context::pickPhysicalDevice(
    const vk::PhysicalDeviceType& preferredType) const
{
    Log::Core::Trace("Picking a physical device...");

    // report required extensions
    const auto extensions = DeviceExtensions();
    for (const auto& ext : extensions)
        Log::Core::Trace("Required device extension: {}", ext);

    // find all physical devices
    const auto allPhysicalDevices = _instance.enumeratePhysicalDevices();
    Log::Core::Info("Found {} physical devices:", allPhysicalDevices.size());

    for (auto&& [i, physicalDevice] : std::ranges::views::enumerate(allPhysicalDevices))
    {
        const auto props = physicalDevice.getProperties();
        Log::Core::Info(
            "  [{}] {} ({}), API version {}.{}.{}",
            i + 1,
            std::string(props.deviceName),
            vk::to_string(props.deviceType),
            vk::apiVersionMajor(props.apiVersion),
            vk::apiVersionMinor(props.apiVersion),
            vk::apiVersionPatch(props.apiVersion));
    }

    struct SuitablePhysicalDevice
    {
        uint32_t i;
        vk::raii::PhysicalDevice physicalDevice;
        vk::PhysicalDeviceProperties properties;
        std::vector<std::string> availableExtensions;
        uint32_t graphicsQueueFamilyIndex;
        uint32_t presentQueueFamilyIndex;
        std::vector<vk::SurfaceFormatKHR> surfaceFormats;
        std::vector<vk::PresentModeKHR> presentModes;
        FeaturesChain features;
    };

    // partition suitable devices by type
    std::array<std::vector<SuitablePhysicalDevice>, 3> physicalDevices;
    size_t binSize = allPhysicalDevices.size() / 3;
    for (auto& bin : physicalDevices)
        bin.reserve(binSize);

    std::map<vk::PhysicalDeviceType, int> binIndex{{
        {vk::PhysicalDeviceType::eDiscreteGpu, 0},
        {vk::PhysicalDeviceType::eIntegratedGpu, 1},
        {vk::PhysicalDeviceType::eCpu, 2},
    }};

    std::array<int, 3> binOrder{};
    switch (preferredType)
    {
    case vk::PhysicalDeviceType::eDiscreteGpu:
        binOrder = {0, 1, 2};
        break;
    case vk::PhysicalDeviceType::eIntegratedGpu:
        binOrder = {1, 0, 2};
        break;
    case vk::PhysicalDeviceType::eCpu:
        binOrder = {2, 1, 0};
        break;
    case vk::PhysicalDeviceType::eOther:
    case vk::PhysicalDeviceType::eVirtualGpu:
        throw std::runtime_error("Refusing to pick a virtual device");
    }

    for (auto&& [i, physicalDevice] : std::ranges::views::enumerate(allPhysicalDevices))
    {
        const auto props = physicalDevice.getProperties();

        // skip headless devices
        if (props.deviceType == vk::PhysicalDeviceType::eOther ||
            props.deviceType == vk::PhysicalDeviceType::eVirtualGpu)
        {
            Log::Core::Trace("Skipping device {}: headless", i + 1);
            continue;
        }

        // check for Vulkan 1.3 support
        if (props.apiVersion < vk::ApiVersion13)
        {
            Log::Core::Trace("Skipping device {}: does not support Vulkan 1.3", i + 1);
            continue;
        }

        // check required device extensions
        const auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties() |
                                         std::ranges::views::transform([](const auto p) {
                                             return std::string(p.extensionName);
                                         }) |
                                         std::ranges::to<std::vector<std::string>>();

        if (!CheckDeviceExtensions(availableExtensions, i))
        {
            Log::Core::Trace("Skipping device {}: missing required extensions", i + 1);
            continue;
        }

        // check for graphics and present queue families
        const auto maybe_indices = [&]() -> std::optional<std::pair<uint32_t, uint32_t>> {
            std::optional<uint32_t> maybe_graphics, maybe_present;
            for (const auto&& [i, family] :
                 std::ranges::views::enumerate(physicalDevice.getQueueFamilyProperties()))
            {
                if (family.queueFlags & vk::QueueFlagBits::eGraphics)
                {
                    if (physicalDevice.getSurfaceSupportKHR(i, *_surface))
                        return std::make_pair(i, i);
                    if (!maybe_graphics)
                        maybe_graphics = i;
                }
                else if (!maybe_present && physicalDevice.getSurfaceSupportKHR(i, *_surface))
                    maybe_present = i;
            }

            if (maybe_graphics && maybe_present)
                return std::make_pair(*maybe_graphics, *maybe_present);

            return std::nullopt;
        }();

        if (!maybe_indices)
        {
            Log::Core::Trace(
                "Skipping device {}: graphics / present operations not supported", i + 1);
            continue;
        }

        const auto& [graphicsQueueFamilyIndex, presentQueueFamilyIndex] = *maybe_indices;

        // check for surface format compatibility
        auto formats = physicalDevice.getSurfaceFormatsKHR(_surface);

        if (formats.empty())
        {
            Log::Core::Trace("Skipping device {}: no compatible surface formats", i + 1);
            continue;
        }

        // check require device features
        const auto features = CreateFeaturesChain(physicalDevice, i);

        if (!features)
        {
            Log::Core::Trace("Skipping device {}: missing required features", i + 1);
            continue;
        }

        // bin suitable candidate
        physicalDevices[binIndex[props.deviceType]].emplace_back(SuitablePhysicalDevice(
            i,
            physicalDevice,
            props,
            availableExtensions,
            graphicsQueueFamilyIndex,
            presentQueueFamilyIndex,
            formats,
            physicalDevice.getSurfacePresentModesKHR(_surface),
            *features));
    }

    // pick the first suitable candidate
    for (const size_t bin : binOrder)
    {
        if (physicalDevices[bin].empty())
            continue;

        for (const auto& pd : physicalDevices[bin])
        {
            Log::Core::Info(
                "Picked physical device {}: {} ({}), API version {}.{}.{}",
                pd.i + 1,
                std::string(pd.properties.deviceName),
                vk::to_string(pd.properties.deviceType),
                vk::apiVersionMajor(pd.properties.apiVersion),
                vk::apiVersionMinor(pd.properties.apiVersion),
                vk::apiVersionPatch(pd.properties.apiVersion));
            Log::Core::Info("  formats: {}", SurfaceFormatNames(pd.surfaceFormats));
            Log::Core::Info("  present modes: {}", PresentModeNames(pd.presentModes));

            // report available extensions
            for (const auto& ext : pd.availableExtensions)
                Log::Core::Trace("Available device extension: {}", ext);

            return PhysicalDevice(
                pd.physicalDevice,
                pd.graphicsQueueFamilyIndex,
                pd.presentQueueFamilyIndex,
                pd.surfaceFormats,
                pd.presentModes,
                pd.features);
        }
    }

    throw std::runtime_error("No suitable physical device found");
}

constexpr std::vector<const char*> Context::DeviceExtensions()
{
    return {
        vk::KHRSwapchainExtensionName,
    };
}

bool Context::CheckDeviceExtensions(const std::vector<std::string>& available, const size_t i)
{
    std::set<std::string> missing(available.begin(), available.end());

    for (const auto& layer : available)
        missing.erase(layer);

    if (!missing.empty())
    {
        for (const auto& ext : missing)
            Log::Core::Warn("Physical device {} missing device extension: {}", i + 1, ext);
        return false;
    }

    return true;
}

std::string Context::SurfaceFormatName(const vk::SurfaceFormatKHR& format)
{
    return vk::to_string(format.format) + "+" + vk::to_string(format.colorSpace);
}

std::string Context::SurfaceFormatNames(const std::vector<vk::SurfaceFormatKHR>& formats)
{
    return formats | std::ranges::views::transform(SurfaceFormatName) |
           std::ranges::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>();
}

std::string Context::PresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes)
{
    const auto modeName = [](const vk::PresentModeKHR& mode) { return vk::to_string(mode); };

    return presentModes | std::ranges::views::transform(modeName) |
           std::ranges::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>();
}

std::optional<Context::FeaturesChain> Context::CreateFeaturesChain(
    const vk::raii::PhysicalDevice& physicalDevice, const size_t i)
{
    const auto [h10, have13, have11, haveEDS] = physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    const auto have10 = h10.features;

    vk::PhysicalDeviceFeatures2 want10;
    vk::PhysicalDeviceVulkan13Features want13;
    vk::PhysicalDeviceVulkan11Features want11;
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT wantEDS;

    bool hasAllRequired = true;

    auto require = [&](const vk::Bool32 source, vk::Bool32& dest, const std::string& name) {
        if (source == vk::True)
            dest = vk::True;
        else
        {
            Log::Core::Trace("Physical device {} missing feature: {}", i + 1, name);
            hasAllRequired = false;
        }
    };

    require(have11.shaderDrawParameters, want11.shaderDrawParameters, "shaderDrawParameters (1.1)");

    require(have13.synchronization2, want13.synchronization2, "synchronization2 (1.3)");
    require(have13.dynamicRendering, want13.dynamicRendering, "dynamicRendering (1.3)");
    require(
        haveEDS.extendedDynamicState, wantEDS.extendedDynamicState, "extendedDynamicState (EDS)");

    if (hasAllRequired)
        return FeaturesChain(want10, want13, want11, wantEDS);

    return std::nullopt;
}

// Logical Device //////////////////////////////////////////////////////////////////////////////////

Context::Device Context::createDevice() const
{
    const float queuePriority = 1.0;

    const auto queueCreateInfos = [&]() -> std::vector<vk::DeviceQueueCreateInfo> {
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = {
            vk::DeviceQueueCreateInfo(
                {}, _physicalDevice.graphicsQueueFamilyIndex, 1, &queuePriority),
        };
        if (_physicalDevice.graphicsQueueFamilyIndex != _physicalDevice.presentQueueFamilyIndex)
            queueCreateInfos.push_back(
                vk::DeviceQueueCreateInfo(
                    {}, _physicalDevice.presentQueueFamilyIndex, 1, &queuePriority));
        return queueCreateInfos;
    }();

    const auto extensions = DeviceExtensions();

    const vk::DeviceCreateInfo createInfo(
        {},
        queueCreateInfos.size(),
        queueCreateInfos.data(),
        0,
        nullptr,
        extensions.size(),
        extensions.data(),
        &_physicalDevice.features.get<vk::PhysicalDeviceFeatures2>().features,
        _physicalDevice.features.get<vk::PhysicalDeviceVulkan13Features>());

    auto device        = vk::raii::Device(*_physicalDevice, createInfo);
    auto graphicsQueue = vk::raii::Queue(device, _physicalDevice.graphicsQueueFamilyIndex, 0);
    auto presentQueue  = vk::raii::Queue(device, _physicalDevice.presentQueueFamilyIndex, 0);

    device.setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT(
            vk::ObjectType::eDevice, reinterpret_cast<uint64_t>(&**device), "Vulkan Device"));

    Log::Core::Info("Logical device created");

    return Device(std::move(device), std::move(graphicsQueue), std::move(presentQueue));
}

// Swap Chain //////////////////////////////////////////////////////////////////////////////////////

Context::SwapChain Context::createSwapChain(SDL_Window* window) const
{
    const auto caps = _physicalDevice->getSurfaceCapabilitiesKHR(_surface);

    // determine swap chain image dimensions
    const auto extent = [&]() -> vk::Extent2D {
        if (caps.currentExtent.width != 0xFFFFFFFF)
            return caps.currentExtent;

        int width = 0, height = 0;
        if (!SDL_GetWindowSizeInPixels(window, &width, &height))
            throw SDLException("Could not determine window size");

        return {
            std::clamp<uint32_t>(width, caps.minImageExtent.width, caps.minImageExtent.height),
            std::clamp<uint32_t>(height, caps.minImageExtent.height, caps.minImageExtent.height),
        };
    }();

    Log::Core::Trace("Surface extent: {}x{}", extent.width, extent.height);

    // choose a surface format
    const auto surfaceFormat = [&]() -> vk::SurfaceFormatKHR {
        // check if device prefers BGR formats
        const auto isNativeBgr = [&]() -> bool {
            for (const auto format : _physicalDevice.surfaceFormats)
            {
                switch (format.format)
                {
                case vk::Format::eR8G8B8A8Unorm:
                case vk::Format::eR8G8B8A8Srgb:
                case vk::Format::eA2R10G10B10UnormPack32:
                    return false;
                case vk::Format::eB8G8R8A8Unorm:
                case vk::Format::eB8G8R8A8Srgb:
                case vk::Format::eA2B10G10R10UnormPack32:
                    return true;
                default:
                    break;
                }
            }
            return false;
        }();

        Log::Core::Trace("Native BGR support: {}", isNativeBgr ? "yes" : "no");

        // for now, hard code client's preferred format and color space
        const auto preferred = vk::SurfaceFormatKHR(
            isNativeBgr ? vk::Format::eB8G8R8A8Unorm : vk::Format::eR8G8B8A8Unorm,
            vk::ColorSpaceKHR::eSrgbNonlinear);

        Log::Core::Trace(
            "Preferred format: {} / {}",
            vk::to_string(preferred.format),
            vk::to_string(preferred.colorSpace));

        Log::Core::Trace("Available formats:");
        for (const auto& format : _physicalDevice.surfaceFormats)
            Log::Core::Trace(
                "  {} / {}", vk::to_string(format.format), vk::to_string(format.colorSpace));

        // check if device supports client's preferred format and color space
        for (const auto& format : _physicalDevice.surfaceFormats)
            if (format.format == preferred.format && format.colorSpace == preferred.colorSpace)
                return format;

        Log::Core::Trace("Could not find the preferred swap chain format and color space");

        // if not, check if device supports client's preferred format with any color space
        for (const auto& format : _physicalDevice.surfaceFormats)
            if (format.format == preferred.format)
                return format;

        Log::Core::Trace("Could not find the preferred swap chain format");

        // Otherwise, default to the first format and color space
        return _physicalDevice.surfaceFormats[0];
    }();

    Log::Core::Trace("Chosen format: {}", SurfaceFormatName(surfaceFormat));

    // choose a present mode
    const auto presentMode = [&]() -> vk::PresentModeKHR {
        const auto& modes = _physicalDevice.presentModes;
        if (std::ranges::find(modes, vk::PresentModeKHR::eMailbox) != modes.end())
            return vk::PresentModeKHR::eMailbox;
        if (std::ranges::find(modes, vk::PresentModeKHR::eImmediate) != modes.end())
            return vk::PresentModeKHR::eImmediate;
        if (std::ranges::find(modes, vk::PresentModeKHR::eFifo) != modes.end())
            return vk::PresentModeKHR::eFifo;
        throw std::runtime_error("Device offers no supported present modes");
    }();

    Log::Core::Trace("Present mode: {}", vk::to_string(presentMode));

    // determine the number of swap chain images
    const auto imageCount = caps.maxImageCount == 0
                                ? caps.minImageCount + 1
                                : std::min(caps.minImageCount + 1, caps.maxImageCount);

    Log::Core::Trace("Number of swap chain images: {}", imageCount);

    // determine unique queue family indices
    const auto queueFamilyIndices = [&]() -> std::set<uint32_t> {
        std::set<uint32_t> indices;
        indices.emplace(_physicalDevice.graphicsQueueFamilyIndex);
        indices.emplace(_physicalDevice.presentQueueFamilyIndex);
        return indices;
    }() | std::ranges::to<std::vector>();

    // create the swap chain
    const auto createInfo = vk::SwapchainCreateInfoKHR(
        {},
        _surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        queueFamilyIndices,
        caps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity
            ? vk::SurfaceTransformFlagBitsKHR::eIdentity
            : caps.currentTransform,
        caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque
            ? vk::CompositeAlphaFlagBitsKHR::eOpaque
            : vk::CompositeAlphaFlagBitsKHR::eInherit,
        presentMode);

    auto swapChain = vk::raii::SwapchainKHR(*_device, createInfo);
    auto images    = swapChain.getImages();

    setDebugName(
        vk::ObjectType::eSwapchainKHR,
        reinterpret_cast<uint64_t>(&**swapChain),
        "Vulkan SwapChain");

    // create a view of each image in the swap chain
    std::vector<vk::raii::ImageView> imageViews;
    for (const auto& [i, image] : std::ranges::views::enumerate(images))
    {
        setDebugName(
            vk::ObjectType::eImage,
            reinterpret_cast<uint64_t>(&*image),
            std::format("SwapChain Image {}", i));

        const vk::ImageViewCreateInfo imageViewCreateInfo(
            {},
            image,
            vk::ImageViewType::e2D,
            surfaceFormat.format,
            {},
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        imageViews.emplace_back(*_device, imageViewCreateInfo);

        setDebugName(
            vk::ObjectType::eImageView,
            reinterpret_cast<uint64_t>(&**imageViews.back()),
            std::format("SwapChain Image View {}", i));
    }

    Log::Core::Info("Swap chain created");

    return SwapChain(std::move(swapChain), images, surfaceFormat, extent, std::move(imageViews));
}

// Pipeline ////////////////////////////////////////////////////////////////////////////////////////

Context::Pipeline Context::createPipeline() const
{
    // create shader module
    const auto shaderModule = createShaderModule(readFile("shaders/slang.slang.spv"));

    // define programmable shader stages
    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
        // vertex shader
        vk::PipelineShaderStageCreateInfo(
            {}, vk::ShaderStageFlagBits::eVertex, shaderModule, "vertMain"),
        // fragment shader
        vk::PipelineShaderStageCreateInfo(
            {}, vk::ShaderStageFlagBits::eFragment, shaderModule, "fragMain"),
    };

    // describe vertex data format
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

    // set drawing primitive type
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
        {}, vk::PrimitiveTopology::eTriangleList);

    // declare rendering viewports and scissors
    vk::PipelineViewportStateCreateInfo viewportState(
        {},
        1,   // viewport count
        {},  // viewports
        1,   // scissor count
        {}); // scissors

    // configure the rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {},
        vk::False,                   // depth clamp enable
        vk::False,                   // discard enable
        vk::PolygonMode::eFill,      // polygon mmode
        vk::CullModeFlagBits::eBack, // coll mode
        vk::FrontFace::eClockwise,   // front face
        vk::False,                   // depth bias enable
        {},                          // depth bias constant factor
        {},                          // depth bias clamp
        1.0F,                        // depth bias slope factor
        1.0F);                       // line width

    // configure multisampling
    vk::PipelineMultisampleStateCreateInfo multisampling(
        {},
        vk::SampleCountFlagBits::e1, // rasterization samples
        vk::False);                  // sample shading enable

    // configure color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment(
        vk::False,                       // blend enable
        {},                              // source color blend factor
        {},                              // destination color blend factor
        {},                              // color blend operation
        {},                              // source alpha blend factor
        {},                              // destination alpha blend factor
        {},                              // alpha blend operation
        vk::ColorComponentFlagBits::eR | // color write mask
            vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA);

    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {},
        vk::False,              // logical operation enable
        vk::LogicOp::eCopy,     // logical operation
        1,                      // attachment count
        &colorBlendAttachment); // attachments

    // declare dynamic states
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicState({}, dynamicStates.size(), dynamicStates.data());

    // define pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {},
        0,   // set layout count
        {},  // set layouts
        0,   // push counstant range count
        {}); // push constant ranges

    vk::raii::PipelineLayout pipelineLayout(*_device, pipelineLayoutInfo);

    // create the graphics pipeline
    vk::PipelineRenderingCreateInfo pipelineRenderingInfo(
        {},                                // view mask
        1,                                 // color attachment count
        &_swapChain.surfaceFormat.format); // color attachment formats

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        shaderStages.size(),     // stage count
        shaderStages.data(),     // stages
        &vertexInputInfo,        // vertex input state
        &inputAssembly,          // input assembly state
        {},                      // tesselation state
        &viewportState,          // viewport state
        &rasterizer,             // rasterization state
        &multisampling,          // multisample state
        nullptr,                 // depth stencil state
        &colorBlending,          // color blend state
        &dynamicState,           // dynamic state
        pipelineLayout,          // layout
        nullptr,                 // render pass
        {},                      // subpass
        {},                      // base pipeline handle
        {},                      // base pipeline index
        &pipelineRenderingInfo); // pNext

    vk::raii::Pipeline graphicsPipeline(*_device, nullptr, pipelineInfo);

    Log::Core::Info("Shader pipeline created");

    return Pipeline(std::move(pipelineLayout), std::move(graphicsPipeline));
}

[[nodiscard]] vk::raii::ShaderModule Context::createShaderModule(
    const std::vector<char>& code) const
{
    return vk::raii::ShaderModule(
        *_device,
        vk::ShaderModuleCreateInfo(
            {}, code.size() * sizeof(char), reinterpret_cast<const uint32_t*>(code.data())));
}

} // namespace VoxelDynamics::Vulkan
