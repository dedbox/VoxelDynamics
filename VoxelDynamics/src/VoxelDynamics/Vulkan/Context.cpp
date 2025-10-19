#include "SDL3/SDL_vulkan.h"

#include "VoxelDynamics/Vulkan/Context.hpp"

namespace VoxelDynamics::Vulkan
{

Context::Context(
    BuildInfo buildInfo_, const std::string& appName, const uint64_t appVersion, Window& window)
    : buildInfo(buildInfo_)
    , _instance(createInstance(appName, appVersion))
    , _surface(createSurface(window))
    , _physicalDevice(pickPhysicalDevice())
    , _device(createDevice())
{
}

// Instance ////////////////////////////////////////////////////////////////////////////////////////

vk::raii::Instance Context::createInstance(
    const std::string& appName, const uint64_t appVersion) const
{
    const vk::ApplicationInfo appInfo(
        appName.c_str(), appVersion, EngineName.c_str(), EngineVersion, vk::ApiVersion13);

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

vk::raii::SurfaceKHR Context::createSurface(Window& window) const
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(*window, *_instance, nullptr, &surface))
        throw SDLException("Could not create Vulkan surface");
    return vk::raii::SurfaceKHR(_instance, surface);
}

// Physical Device /////////////////////////////////////////////////////////////////////////////////

const PhysicalDevice Context::pickPhysicalDevice() const
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
        uint32_t graphicsIndex, presentIndex, transferIndex;
        std::vector<vk::SurfaceFormatKHR> surfaceFormats;
        std::vector<vk::PresentModeKHR> presentModes;
        PhysicalDevice::FeaturesChain features;
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

        const auto& [graphicsIndex, presentIndex] = *maybe_indices;

        // check for transfer queue family
        const auto maybe_transfer = [&]() -> std::optional<uint32_t> {
            std::optional<uint32_t> maybe_transfer;
            for (const auto&& [i, family] :
                 std::ranges::views::enumerate(physicalDevice.getQueueFamilyProperties()))
                if (family.queueFlags & vk::QueueFlagBits::eTransfer)
                {
                    if (family.queueFlags & vk::QueueFlagBits::eGraphics ||
                        physicalDevice.getSurfaceSupportKHR(i, *_surface))
                    {
                        if (!maybe_transfer)
                            maybe_transfer = i;
                    }
                    else
                        return i;
                }
            return maybe_transfer;
        }();

        if (!maybe_transfer)
        {
            Log::Core::Trace("Skipping device {}: transfer operations not supported", i + 1);
            continue;
        }

        const auto& transferIndex = *maybe_transfer;

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
            graphicsIndex,
            presentIndex,
            transferIndex,
            formats,
            physicalDevice.getSurfacePresentModesKHR(_surface),
            *features));
    }

    // pick the first suitable candidate
    for (const size_t bin : binOrder)
    {
        if (physicalDevices[bin].empty())
            continue;

        const auto& pd = physicalDevices[bin].front();

        // determine unique queue family indices
        const auto uniqueIndices = [&]() -> std::set<uint32_t> {
            std::set<uint32_t> indices;
            indices.emplace(_physicalDevice.graphicsIndex);
            indices.emplace(_physicalDevice.presentIndex);
            indices.emplace(_physicalDevice.transferIndex);
            return indices;
        }() | std::ranges::to<std::vector>();

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
        Log::Core::Info("  unique queue families: {}", uniqueIndices.size());

        for (const auto& ext : pd.availableExtensions)
            Log::Core::Trace("Available device extension: {}", ext);

        return PhysicalDevice(
            pd.physicalDevice,
            pd.graphicsIndex,
            pd.presentIndex,
            pd.transferIndex,
            uniqueIndices,
            pd.surfaceFormats,
            pd.presentModes,
            pd.features);
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
    return vk::to_string(format.format) + " / " + vk::to_string(format.colorSpace);
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

std::optional<PhysicalDevice::FeaturesChain> Context::CreateFeaturesChain(
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
        return PhysicalDevice::FeaturesChain(want10, want13, want11, wantEDS);

    return std::nullopt;
}

// Logical Device //////////////////////////////////////////////////////////////////////////////////

Device Context::createDevice() const
{
    const float queuePriority   = 1.0;
    const auto queueCreateInfos = [&]() -> std::vector<vk::DeviceQueueCreateInfo> {
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = {
            vk::DeviceQueueCreateInfo({}, _physicalDevice.graphicsIndex, 1, &queuePriority),
        };
        if (_physicalDevice.graphicsIndex != _physicalDevice.presentIndex)
            queueCreateInfos.push_back(
                vk::DeviceQueueCreateInfo({}, _physicalDevice.presentIndex, 1, &queuePriority));
        if (_physicalDevice.graphicsIndex != _physicalDevice.transferIndex)
            queueCreateInfos.push_back(
                vk::DeviceQueueCreateInfo({}, _physicalDevice.transferIndex, 1, &queuePriority));
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
    auto graphicsQueue = vk::raii::Queue(device, _physicalDevice.graphicsIndex, 0);
    auto presentQueue  = vk::raii::Queue(device, _physicalDevice.presentIndex, 0);
    auto transferQueue = vk::raii::Queue(device, _physicalDevice.transferIndex, 0);

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

    device.setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT(
            vk::ObjectType::eQueue,
            reinterpret_cast<uint64_t>(&**transferQueue),
            "Transfer Queue"));

    Log::Core::Info("Logical device created");

    return Device(
        std::move(device),
        std::move(graphicsQueue),
        std::move(presentQueue),
        std::move(transferQueue));
}

void Context::setDebugName(vk::ObjectType type, void* handle, const std::string& name) const
{
    _device->setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT(type, reinterpret_cast<uint64_t>(handle), name.c_str()));
}

} // namespace VoxelDynamics::Vulkan
