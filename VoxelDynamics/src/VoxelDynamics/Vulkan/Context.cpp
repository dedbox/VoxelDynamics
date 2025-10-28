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

PhysicalDevice Context::pickPhysicalDevice() const
{
    Log::Core::Trace("Picking a physical device...");

    // report required extensions
    const auto extensions = PhysicalDevice::RequiredDeviceExtensions();
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
        PhysicalDevice physicalDevice;
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

    for (auto&& [i, raw_physicalDevice] : std::ranges::views::enumerate(allPhysicalDevices))
    {
        PhysicalDevice physicalDevice(raw_physicalDevice, _surface);

        // skip headless devices
        if (physicalDevice.properties.deviceType == vk::PhysicalDeviceType::eOther ||
            physicalDevice.properties.deviceType == vk::PhysicalDeviceType::eVirtualGpu)
        {
            Log::Core::Trace("Skipping device {}: headless", i + 1);
            continue;
        }

        // check for Vulkan 1.3 support
        if (physicalDevice.properties.apiVersion < vk::ApiVersion13)
        {
            Log::Core::Trace("Skipping device {}: does not support Vulkan 1.3", i + 1);
            continue;
        }

        // check required device extensions
        if (!physicalDevice.checkDeviceExtensions())
        {
            Log::Core::Trace("Skipping device {}: missing required extensions", i + 1);
            continue;
        }

        // check queue families
        if (!physicalDevice.index)
        {
            Log::Core::Trace(
                "Skipping device {}: graphics / present operations not supported", i + 1);
            continue;
        }

        // check for surface format compatibility
        if (physicalDevice.surfaceFormats.empty())
        {
            Log::Core::Trace("Skipping device {}: no compatible surface formats", i + 1);
            continue;
        }

        // check require device features
        if (!physicalDevice.features)
        {
            Log::Core::Trace("Skipping device {}: missing required features", i + 1);
            continue;
        }

        // bin suitable candidate
        physicalDevices[binIndex[physicalDevice.properties.deviceType]].emplace_back(
            i, std::move(physicalDevice));
    }

    // pick the first suitable candidate
    for (const size_t bin : binOrder)
    {
        if (physicalDevices[bin].empty())
            continue;

        auto& pd = physicalDevices[bin].front();

        Log::Core::Info(
            "Picked physical device {}: {} ({}), API version {}.{}.{}",
            pd.i + 1,
            std::string(pd.physicalDevice.properties.deviceName),
            vk::to_string(pd.physicalDevice.properties.deviceType),
            vk::apiVersionMajor(pd.physicalDevice.properties.apiVersion),
            vk::apiVersionMinor(pd.physicalDevice.properties.apiVersion),
            vk::apiVersionPatch(pd.physicalDevice.properties.apiVersion));
        Log::Core::Info("  formats: {}", SurfaceFormatNames(pd.physicalDevice.surfaceFormats));
        Log::Core::Info("  present modes: {}", PresentModeNames(pd.physicalDevice.presentModes));
        Log::Core::Info("  transfer queue family index: {}", pd.physicalDevice.index->transfer);
        Log::Core::Info("  graphics queue family index: {}", pd.physicalDevice.index->graphics);
        Log::Core::Info("  present queue family index: {}", pd.physicalDevice.index->present);

        for (const auto& ext : pd.physicalDevice.availableExtensions)
            Log::Core::Trace("Available device extension: {}", ext);

        return std::move(pd.physicalDevice);
    }

    throw std::runtime_error("No suitable physical device found");
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

// Logical Device //////////////////////////////////////////////////////////////////////////////////

Device Context::createDevice() const
{
    const float queuePriority   = 1.0;
    const auto queueCreateInfos = [&]() -> std::vector<vk::DeviceQueueCreateInfo> {
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(3);

        queueCreateInfos.push_back(
            vk::DeviceQueueCreateInfo({}, _physicalDevice.index->graphics, 1, &queuePriority));

        if (_physicalDevice.index->present != _physicalDevice.index->graphics)
        {
            const auto createInfo =
                vk::DeviceQueueCreateInfo({}, _physicalDevice.index->present, 1, &queuePriority);
            queueCreateInfos.push_back(createInfo);
        }

        if (_physicalDevice.index->transfer != _physicalDevice.index->graphics)
        {
            const auto createInfo =
                vk::DeviceQueueCreateInfo({}, _physicalDevice.index->transfer, 1, &queuePriority);
            queueCreateInfos.push_back(createInfo);
        }

        return queueCreateInfos;
    }();

    const auto extensions = PhysicalDevice::RequiredDeviceExtensions();
    const auto features   = _physicalDevice.features->get<vk::PhysicalDeviceFeatures2>().features;
    const auto features13 = _physicalDevice.features->get<vk::PhysicalDeviceVulkan13Features>();

    const vk::DeviceCreateInfo createInfo(
        {},
        queueCreateInfos.size(),
        queueCreateInfos.data(),
        0,
        nullptr,
        extensions.size(),
        extensions.data(),
        &features,
        features13);

    auto device        = vk::raii::Device(_physicalDevice.physicalDevice, createInfo);
    auto graphicsQueue = vk::raii::Queue(device, _physicalDevice.index->graphics, 0);
    auto presentQueue  = vk::raii::Queue(device, _physicalDevice.index->present, 0);
    auto transferQueue = vk::raii::Queue(device, _physicalDevice.index->transfer, 0);

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
