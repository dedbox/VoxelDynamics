#include "VoxelDynamics/Renderer/Vulkan/Context.hpp"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_vulkan.h"

#include "VoxelDynamics/Core/Util.hpp"

namespace VoxelDynamics::Vulkan
{

Context::Context(SDL_Window* window, const BuildInfo& buildInfo_) // NOLINT
    : buildInfo(buildInfo_)
    , _instance(createInstance())
    , _surface(createSurface(window))
    , _physicalDevice(pickPhysicalDevice())
    , _device(createDevice())
    , _swapChain(createSwapChain(window))
{
}

void Context::wait() const
{
    _device->waitIdle();
}

void Context::recreateSwapChain(SDL_Window* window)
{
    wait();
    cleanupSwapChain();
    _swapChain = createSwapChain(window);
}

// Instance ////////////////////////////////////////////////////////////////////////////////////////

vk::raii::Instance Context::createInstance() const
{
    const vk::ApplicationInfo appInfo(
        buildInfo.appName.c_str(),
        buildInfo.appVersion,
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
        Log::Core::Debug("{}", pCallbackData->pMessage);
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

Context::PhysicalDevice Context::pickPhysicalDevice() const
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
    switch (buildInfo.preferredDeviceType)
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

    device.setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT(
            vk::ObjectType::eQueue,
            reinterpret_cast<uint64_t>(&**graphicsQueue),
            "Graphics Queue"));

    device.setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT(
            vk::ObjectType::eQueue, reinterpret_cast<uint64_t>(&**presentQueue), "Present Queue"));

    Log::Core::Info("Logical device created");

    return Device(std::move(device), std::move(graphicsQueue), std::move(presentQueue));
}

// Swap Chain //////////////////////////////////////////////////////////////////////////////////////

Context::SwapChain Context::createSwapChain(SDL_Window* window) const
{
    Log::Core::Info("Querying surface capabilities:");

    const auto caps = _physicalDevice->getSurfaceCapabilitiesKHR(_surface);

    // determine swap chain image dimensions
    const auto extent = [&]() -> vk::Extent2D {
        if (caps.currentExtent.width != 0xFFFFFFFF)
            return caps.currentExtent;

        // TODO do we need to handle window minimize separately?
        int width = 0, height = 0;
        if (!SDL_GetWindowSizeInPixels(window, &width, &height))
            throw SDLException("Could not determine window size");

        return {
            std::clamp<uint32_t>(width, caps.minImageExtent.width, caps.minImageExtent.height),
            std::clamp<uint32_t>(height, caps.minImageExtent.height, caps.minImageExtent.height),
        };
    }();

    Log::Core::Info("  extent: {}x{}", extent.width, extent.height);

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

        Log::Core::Info("  native BGR: {}", isNativeBgr ? "yes" : "no");

        // for now, hard code client's preferred format and color space
        const auto preferred = vk::SurfaceFormatKHR(
            isNativeBgr ? vk::Format::eB8G8R8A8Unorm : vk::Format::eR8G8B8A8Unorm,
            vk::ColorSpaceKHR::eSrgbNonlinear);

        Log::Core::Info(
            "  preferred format: {} / {}",
            vk::to_string(preferred.format),
            vk::to_string(preferred.colorSpace));

        Log::Core::Info("  available formats:");
        for (const auto& format : _physicalDevice.surfaceFormats)
            Log::Core::Info(
                "    {} / {}", vk::to_string(format.format), vk::to_string(format.colorSpace));

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

    // determine the number of swap chain images
    const auto imageCount = caps.maxImageCount == 0
                                ? caps.minImageCount + 1
                                : std::min(caps.minImageCount + 1, caps.maxImageCount);

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
    imageViews.reserve(images.size());
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

    // create a semaphore for each image in the swap chain
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    imageAvailableSemaphores.reserve(images.size());

    vk::SemaphoreCreateInfo semaphoreCreateInfo{};

    for (const auto i : std::ranges::views::iota(0U, images.size()))
    {
        imageAvailableSemaphores.emplace_back(*_device, semaphoreCreateInfo);

        setDebugName(
            vk::ObjectType::eSemaphore,
            reinterpret_cast<uint64_t>(&**imageAvailableSemaphores[i]),
            std::format("Image Available Semaphore {}", i));
    }

    Log::Core::Info("Swap chain created:");
    Log::Core::Info("  number of swap chain images: {}", imageCount);
    Log::Core::Info("  chosen format: {}", SurfaceFormatName(surfaceFormat));
    Log::Core::Info("  present mode: {}", vk::to_string(presentMode));

    return SwapChain(
        std::move(swapChain),
        images,
        surfaceFormat,
        extent,
        std::move(imageViews),
        std::move(imageAvailableSemaphores));
}

// Pipeline ////////////////////////////////////////////////////////////////////////////////////////

Context::Pipeline Context::createGraphicsPipeline(const std::string& spvFilePath) const
{
    // load SPIR-V file
    const std::vector<char> code = readFile(spvFilePath);

    // create shader module
    const auto shaderModule = vk::raii::ShaderModule(
        *_device,
        vk::ShaderModuleCreateInfo(
            {}, code.size() * sizeof(char), reinterpret_cast<const uint32_t*>(code.data())));

    // define programmable shader stages
    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
        // vertex shader
        vk::PipelineShaderStageCreateInfo(
            {}, vk::ShaderStageFlagBits::eVertex, shaderModule, "vertMain"),
        // fragment shader
        vk::PipelineShaderStageCreateInfo(
            {}, vk::ShaderStageFlagBits::eFragment, shaderModule, "fragMain"),
    };

    setDebugName(
        vk::ObjectType::eShaderModule,
        reinterpret_cast<uint64_t>(&**shaderModule),
        "Shader Module");

    Log::Core::Info("Loaded SPIR-V bytecode file `{}'", spvFilePath);

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

    setDebugName(
        vk::ObjectType::ePipelineLayout,
        reinterpret_cast<uint64_t>(&**pipelineLayout),
        "Pipeline Layout");

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

    setDebugName(
        vk::ObjectType::ePipeline,
        reinterpret_cast<uint64_t>(&**graphicsPipeline),
        "Graphics Pipeline");

    Log::Core::Info("Shader pipeline created");

    return Pipeline(std::move(pipelineLayout), std::move(graphicsPipeline));
}

// Frames //////////////////////////////////////////////////////////////////////////////////////////

Context::Frames Context::createFrames() const
{
    std::vector<Frame> frames;
    frames.reserve(buildInfo.maxFramesInFlight);

    vk::CommandPoolCreateInfo poolCreateInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        _physicalDevice.graphicsQueueFamilyIndex);

    vk::SemaphoreCreateInfo semaphoreCreateInfo{};

    for (const size_t i : std::ranges::views::iota(0, buildInfo.maxFramesInFlight))
    {
        // create command pool
        vk::raii::CommandPool pool(*_device, poolCreateInfo);

        setDebugName(
            vk::ObjectType::eCommandPool,
            reinterpret_cast<uint64_t>(&**pool),
            std::format("Command Pool {}", i));

        // allocate command buffer from the pool
        vk::CommandBufferAllocateInfo bufferAllocInfo(*pool, vk::CommandBufferLevel::ePrimary, 1);

        vk::raii::CommandBuffer buffer =
            std::move(_device->allocateCommandBuffers(bufferAllocInfo).front());

        setDebugName(
            vk::ObjectType::eCommandBuffer,
            reinterpret_cast<uint64_t>(&**buffer),
            std::format("Command Buffer {}", i));

        // create semaphore
        vk::raii::Semaphore renderFinishSemaphore(*_device, semaphoreCreateInfo);

        setDebugName(
            vk::ObjectType::eSemaphore,
            reinterpret_cast<uint64_t>(&**renderFinishSemaphore),
            std::format("Render Finish Semaphore {}", i));

        // create fence in signalled state
        vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
        vk::raii::Fence inFlightFence(*_device, fenceCreateInfo);

        setDebugName(
            vk::ObjectType::eFence,
            reinterpret_cast<uint64_t>(&**inFlightFence),
            std::format("In Flight Fence {}", i));

        frames.emplace_back(
            std::move(pool),
            std::move(buffer),
            std::move(renderFinishSemaphore),
            std::move(inFlightFence));
    }

    return Frames(std::move(frames), 0);
}

void Context::destroyFrames(Frames& frames) const
{
    const auto fences = frames.frames |
                        std::ranges::views::transform(
                            [](const auto& frame) -> vk::Fence { return *frame.inFlightFence; }) |
                        std::ranges::to<std::vector>();

    if (_device->waitForFences(fences, vk::True, std::numeric_limits<uint64_t>::max()) !=
        vk::Result::eSuccess)
        throw std::runtime_error("Failed ot wait for Vulkan fences");
}

void Context::drawCurrentFrame(SDL_Window* window, Frames& frames, Pipeline& pipeline)
{
    Frame& frame = frames.frames[frames.currentFrame];

    // wait for the current frame to become available
    while (vk::Result::eTimeout ==
           _device->waitForFences(
               *frame.inFlightFence, vk::True, std::numeric_limits<uint64_t>::max()))
        ;

    // acquire the next swap chain image
    vk::Result result{};
    uint32_t imageIndex{};
    try
    {
        std::tie(result, imageIndex) = _swapChain->acquireNextImage(
            std::numeric_limits<uint64_t>::max(), frame.imageAvailableSemaphore, nullptr);
    }
    catch (const vk::OutOfDateKHRError& e)
    {
        recreateSwapChain(window);
        return;
    }

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain(window);
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        throw std::runtime_error("Could not acquire the next swap chain image");

    Log::Core::Trace("acquired swap chain image {}", imageIndex);

    // reset fences (now that we know we will be submitting work with it)
    _device->resetFences(*frame.inFlightFence);

    // reset the current command pool (and implicitly the command buffer)
    frame.pool.reset();

    // record the current command buffer
    recordCommandBuffer(frame, imageIndex, pipeline);

    // submit the command buffer
    vk::PipelineStageFlags waitDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

    const vk::SubmitInfo submitInfo(
        1,                                                  // wait semaphore count
        &*frame.imageAvailableSemaphore,                    // wait semaphores
        &waitDstStageMask,                                  // wait destination stage mask
        1,                                                  // command buffer count
        &*frame.buffer,                                     // command buffers
        1,                                                  // signal semaphore count
        &*_swapChain.renderFinishedSemaphores[imageIndex]); // signal semaphores

    _device.graphicsQueue.submit(submitInfo, *frame.inFlightFence);

    // present the image
    const vk::PresentInfoKHR presentInfo(
        1,                                                 // wait semaphore count
        &*_swapChain.renderFinishedSemaphores[imageIndex], // wait semaphores
        1,                                                 // swap chain count
        &**_swapChain,                                     // swap chains
        &imageIndex);                                      // image indices

    try
    {
        result = _device.graphicsQueue.presentKHR(presentInfo);
    }
    catch (const vk::OutOfDateKHRError& e)
    {
        recreateSwapChain(window);
    }

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR ||
        _swapChain.frameBufferResized)
    {
        recreateSwapChain(window);
    }
    else if (result != vk::Result::eSuccess)
        throw std::runtime_error("Could not present swap chain image");

    // move to the next frame
    frames.currentFrame = (frames.currentFrame + 1) % buildInfo.maxFramesInFlight;
}

void Context::recordCommandBuffer(Frame& frame, uint32_t imageIndex, Pipeline& pipeline)
{
    // begin recording
    frame.buffer.begin({});

    // transition swap chain image to color attachment layout
    transitionImageLayout(
        frame.buffer,
        _swapChain.images[imageIndex],
        vk::ImageLayout::eUndefined,                         // old layout
        vk::ImageLayout::eColorAttachmentOptimal,            // new layout
        {},                                                  // source aspect mask (no need to wait)
        vk::AccessFlagBits2::eColorAttachmentWrite,          // destination aspect mask
        vk::PipelineStageFlagBits2::eTopOfPipe,              // source stage mask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput); // destination stage mask

    // configure the color attachment
    vk::ClearValue clearColor = vk::ClearColorValue(0.0F, 0.0F, 0.0F, 1.0F);
    vk::RenderingAttachmentInfo attachmentInfo(
        _swapChain.imageViews[imageIndex],        // image view
        vk::ImageLayout::eColorAttachmentOptimal, // image layout
        vk::ResolveModeFlagBits::eNone,           // resolve mode
        {},                                       // resolve image view
        vk::ImageLayout::eUndefined,              // resolve image layout
        vk::AttachmentLoadOp::eClear,             // load operation
        vk::AttachmentStoreOp::eStore,            // store operation
        clearColor);                              // clear value

    // configure dynamic rendering
    vk::RenderingInfo renderingInfo(
        {},
        vk::Rect2D({0, 0}, _swapChain.extent), // render area
        1,                                     // layer count
        {},                                    // view mask
        1,                                     // color attachment count
        &attachmentInfo);                      // color attachments

    // begin rendering
    frame.buffer.beginRendering(renderingInfo);

    // bind the graphics pipeline
    frame.buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.graphics);

    // supply dynamic rendering data
    frame.buffer.setViewport(
        0,
        vk::Viewport(
            0.0F,
            0.0F,
            static_cast<float>(_swapChain.extent.width),
            static_cast<float>(_swapChain.extent.height),
            0.0F,
            1.0F));
    frame.buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swapChain.extent));

    // issue draw commands
    frame.buffer.draw(
        3,  // vertex count
        1,  // instance count
        0,  // first vertex
        0); // first instance

    // end rendering
    frame.buffer.endRendering();

    // transition swap chain image to present layout
    transitionImageLayout(
        frame.buffer,
        _swapChain.images[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal,           // old layout
        vk::ImageLayout::ePresentSrcKHR,                    // new layout
        vk::AccessFlagBits2::eColorAttachmentWrite,         // source access mask
        {},                                                 // destination access mask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // source stage
        vk::PipelineStageFlagBits2::eBottomOfPipe);         // destination stage

    frame.buffer.end();
}

void Context::transitionImageLayout(
    vk::raii::CommandBuffer& buffer,
    vk::Image& image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask)
{
vk:
    vk::ImageSubresourceRange subresourceRange(
        vk::ImageAspectFlagBits::eColor, // aspect mask
        0,                               // base mip level
        1,                               // level count
        0,                               // base array layer
        1);                              // layer count

    vk::ImageMemoryBarrier2 barrier(
        srcStageMask,
        srcAccessMask,
        dstStageMask,
        dstAccessMask,
        oldLayout,
        newLayout,
        vk::QueueFamilyIgnored, // source queue family index
        vk::QueueFamilyIgnored, // destination queue family index
        image,
        subresourceRange);

    vk::DependencyInfo dependencyInfo(
        {},
        {},        // memory barrier count
        {},        // memory barriers
        {},        // buffer memory barrier count
        {},        // buffer memory barriers
        1,         // image memory barrier count
        &barrier); // image memory barriers

    buffer.pipelineBarrier2(dependencyInfo);
}

void Context::cleanupSwapChain()
{
    _swapChain.renderFinishedSemaphores.clear();
    _swapChain.imageViews.clear();
    _swapChain.swapChain = nullptr;
}

void Context::requestResize()
{
    _swapChain.frameBufferResized = true;
}

} // namespace VoxelDynamics::Vulkan
