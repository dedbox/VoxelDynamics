#include "VoxelDynamics/Renderer/VulkanContext.hpp"

namespace VoxelDynamics
{

VulkanContext::VulkanContext(const Window& window, const CreateInfo& createInfo)
    : _instance(createInstance(createInfo))
    , _surface(createSurface(window))
    , _physicalDevice(pickPhysicalDevice(createInfo))
    , _device(CreateDevice())
{
}

// CreateInstance //////////////////////////////////////////////////////////////////////////////////

vk::raii::Instance VulkanContext::createInstance(const CreateInfo& createInfo)
{
    vk::ApplicationInfo appInfo(
        createInfo.appName.c_str(),
        createInfo.appVersion,
        EngineName.c_str(),
        EngineVersion,
        VK_API_VERSION_1_3);

    const auto layers     = ValidationLayers();
    const auto extensions = InstanceExtensions();

    CheckValidationLayers(layers);
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

std::vector<const char*> VulkanContext::ValidationLayers()
{
    if constexpr (is_debugging_enabled)
        return {
            // "VK_LAYER_LUNARG_api_dump",
            "VK_LAYER_KHRONOS_validation",
        };
    else
        return {};
}

std::vector<const char*> VulkanContext::InstanceExtensions()
{
    uint32_t count; // NOLINT
    const auto raw_extensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extensions(raw_extensions, raw_extensions + count); // NOLINT
    if (is_debugging_enabled)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return extensions;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanContext::DebugUtilsMessengerCallback(
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

vk::DebugUtilsMessengerCreateInfoEXT VulkanContext::DebugUtilsMessengerCreateInfoEXT()
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

void VulkanContext::CheckValidationLayers(const std::vector<const char*>& layers)
{
    auto required =
        layers | std::ranges::views::transform([](const auto layer) { return std::string(layer); });

    for (const auto& layer : required)
        Log::Core::Trace("Required layer: {}", layer);

    auto available =
        vk::enumerateInstanceLayerProperties() |
        std::ranges::views::transform([](const auto p) { return std::string(p.layerName); });

    for (const auto& layer : available)
        Log::Core::Trace("Available layer: {}", layer);

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

void VulkanContext::CheckInstanceExtensions(const std::vector<const char*>& extensions)
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

// CreateSurface ///////////////////////////////////////////////////////////////////////////////////

vk::raii::SurfaceKHR VulkanContext::createSurface(const Window& window)
{
    return vk::raii::SurfaceKHR(_instance, window.createSurface(*_instance));
}

// PickPhysicaldevice //////////////////////////////////////////////////////////////////////////////

VulkanContext::PhysicalDevice VulkanContext::pickPhysicalDevice(const CreateInfo& createInfo)
{
    // Get all physical devices
    vk::raii::PhysicalDevices allPhysicalDevices(_instance);

    Log::Core::Info("Found {} physical devices:", allPhysicalDevices.size());

    // Partition by device type, ignoring unsuitable candidates
    std::array<std::vector<std::pair<int, vk::raii::PhysicalDevice>>, 3> physicalDevices;
    size_t binSize = allPhysicalDevices.size() / 3;
    for (auto& vec : physicalDevices)
        vec.reserve(binSize);

    std::map<vk::PhysicalDeviceType, int> binIndex{{
        {vk::PhysicalDeviceType::eDiscreteGpu, 0},
        {vk::PhysicalDeviceType::eIntegratedGpu, 1},
        {vk::PhysicalDeviceType::eCpu, 2},
    }};

    for (auto&& [i, physicalDevice] : std::ranges::views::enumerate(allPhysicalDevices))
    {
        const auto props = physicalDevice.getProperties();

        Log::Core::Info(
            "  [{}] {} ({})",
            i + 1,
            std::string(props.deviceName),
            vk::to_string(props.deviceType));

        if (props.deviceType == vk::PhysicalDeviceType::eVirtualGpu ||
            props.deviceType == vk::PhysicalDeviceType::eOther)
            continue;

        if (props.apiVersion < EngineApiVersion)
            continue;

        // NOLINTNEXTLINE
        physicalDevices[binIndex[props.deviceType]].emplace_back(i, std::move(physicalDevice));
    }

    // Determine the preferred order of device types
    std::array<int, 3> binOrder; // NOLINT
    switch (createInfo.preferredDeviceType)
    {
    case DeviceType::Discrete:
        binOrder = {0, 1, 2};
        break;
    case DeviceType::Integrated:
        binOrder = {1, 0, 2};
        break;
    case DeviceType::Software:
        binOrder = {2, 1, 0};
        break;
    case DeviceType::Virtual:
        Log::Core::Assert(false, "Refusing to pick a virtual device");
        break;
    }

    const auto deviceExtensions = DeviceExtensions(createInfo.deviceExtensions);

    // Pick the first suitable candidate, starting with the preferred type
    for (const size_t bin : binOrder)
        for (auto& [i, physicalDevice] : physicalDevices[bin]) // NOLINT
        {
            const auto queueFamilyIndices = findQueueFamilyIndices(physicalDevice);
            if (!queueFamilyIndices)
                continue;

            Log::Core::Trace("Checking device {}", i + 1);

            const auto formats = physicalDevice.getSurfaceFormatsKHR(_surface);
            if (!formats.empty() && CheckDeviceExtensions(physicalDevice, deviceExtensions) &&
                CheckDeviceFeatures(physicalDevice))
            {
                const auto presentModes = physicalDevice.getSurfacePresentModesKHR(_surface);
                const auto props        = physicalDevice.getProperties();

                Log::Core::Info(
                    "Picked physical device {}: {} ({})",
                    i + 1,
                    std::string(props.deviceName),
                    vk::to_string(props.deviceType));
                Log::Core::Info("  graphics queue family index: {}", queueFamilyIndices->first);
                Log::Core::Info("  compute queue family index: {}", queueFamilyIndices->second);
                Log::Core::Info("  formats: {}", SurfaceFormatNames(formats));
                Log::Core::Info("  present modes: {}", SufacePresentModeNames(presentModes));

                return PhysicalDevice(
                    std::move(physicalDevice),
                    queueFamilyIndices->first,
                    queueFamilyIndices->second,
                    formats,
                    presentModes);
            }
        }

    throw std::runtime_error("No suitable physical device found");
}

std::optional<std::pair<uint32_t, uint32_t>> VulkanContext::findQueueFamilyIndices(
    const vk::raii::PhysicalDevice& physicalDevice)
{
    const auto families = physicalDevice.getQueueFamilyProperties();

    std::optional<uint32_t> graphics, compute, maybeCompute;
    for (const auto&& [index, family] : std::ranges::views::enumerate(families))
    {
        if (!graphics && family.queueFlags & vk::QueueFlagBits::eGraphics &&
            physicalDevice.getSurfaceSupportKHR(index, _surface))
            graphics = index;

        // try to find a dedicated compute queue
        if (!compute && family.queueFlags & vk::QueueFlagBits::eCompute)
        {
            if (!(family.queueFlags & vk::QueueFlagBits::eGraphics))
                compute = index;
            else if (!maybeCompute)
                maybeCompute = index;
        }

        //  if no dedicated compute queue, use shared compute queue if one was found
        if (!compute && maybeCompute)
            compute = maybeCompute;

        if (graphics && compute)
            return std::make_pair(*graphics, *compute);
    }

    return std::nullopt;
}

std::vector<const char*> VulkanContext::DeviceExtensions(
    const std::vector<const char*>& deviceExtensions)
{
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    extensions.append_range(deviceExtensions);
    return extensions;
}

bool VulkanContext::CheckDeviceExtensions(
    const vk::raii::PhysicalDevice& physicalDevice, const std::vector<const char*>& extensions)
{
    for (const auto& ext : extensions)
        Log::Core::Trace("Required device extension: {}", ext);

    const auto available =
        physicalDevice.enumerateDeviceExtensionProperties() |
        std::ranges::views::transform([](const auto p) { return std::string(p.extensionName); });

    for (const auto& ext : available)
        Log::Core::Trace("Available device extension: {}", ext);

    std::set<std::string> missing(extensions.begin(), extensions.end());
    for (const auto& layer : available)
        missing.erase(layer);

    if (!missing.empty())
    {
        for (const auto& ext : missing)
            Log::Core::Warn("Unsupported device extension: {}", ext);
        return false;
    }

    Log::Core::Trace("All required device extensions are available");

    return true;
}

bool VulkanContext::CheckDeviceFeatures(const vk::raii::PhysicalDevice& physicalDevice)
{
    const auto features = physicalDevice.getFeatures();
    return true;
}

std::string VulkanContext::SurfaceFormatNames(const std::vector<vk::SurfaceFormatKHR>& formats)
{
    const auto formatName = [](const vk::SurfaceFormatKHR& format) {
        return vk::to_string(format.format) + "+" + vk::to_string(format.colorSpace);
    };

    return formats | std::ranges::views::transform(formatName) |
           std::ranges::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>();
}

std::string VulkanContext::SufacePresentModeNames(
    const std::vector<vk::PresentModeKHR>& presentModes)
{
    const auto modeName = [](const vk::PresentModeKHR& mode) { return vk::to_string(mode); };

    return presentModes | std::ranges::views::transform(modeName) |
           std::ranges::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>();
}

// CreateDevice
// ////////////////////////////////////////////////////////////////////////////////////

vk::raii::Device VulkanContext::CreateDevice()
{
    return nullptr;
}

} // namespace VoxelDynamics
