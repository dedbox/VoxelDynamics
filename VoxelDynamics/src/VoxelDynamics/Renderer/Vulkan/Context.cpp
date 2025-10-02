#include "VoxelDynamics/Renderer/Vulkan/Context.hpp"

namespace VoxelDynamics::Vulkan
{

Context::Context(const Window& window, const CreateInfo& createInfo)
    : _instance(createInstance(createInfo))
    , _surface(vk::raii::SurfaceKHR(_instance, window.createSurface(*_instance)))
    , _physicalDevice(
          PhysicalDevice::Create(
              _instance, _surface, createInfo.preferredDeviceType, createInfo.deviceExtensions))
    , _device(CreateDevice(createInfo.deviceExtensions))
{
}

// CreateInstance //////////////////////////////////////////////////////////////////////////////////

vk::raii::Instance Context::createInstance(const CreateInfo& createInfo)
{
    vk::ApplicationInfo appInfo(
        createInfo.appName.c_str(),
        createInfo.appVersion,
        EngineName.c_str(),
        EngineVersion,
        EngineApiVersion);

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

std::vector<const char*> Context::ValidationLayers()
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
    uint32_t count; // NOLINT
    const auto raw_extensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extensions(raw_extensions, raw_extensions + count); // NOLINT
    if (is_debugging_enabled)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return extensions;
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

vk::DebugUtilsMessengerCreateInfoEXT Context::DebugUtilsMessengerCreateInfoEXT()
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

void Context::CheckValidationLayers(const std::vector<const char*>& layers)
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

// CreateDevice
// ////////////////////////////////////////////////////////////////////////////////////

Context::Device Context::CreateDevice(const std::vector<const char*>& deviceExtensions)
{
    const float queuePriority = 1.0F;

    const std::array<vk::DeviceQueueCreateInfo, 2> queueCreateInfo{
        vk::DeviceQueueCreateInfo({}, _physicalDevice.graphicsQueueFamilyIndex, 1, &queuePriority),
        vk::DeviceQueueCreateInfo({}, _physicalDevice.computeQueueFamilyIndex, 1, &queuePriority),
    };

    const uint32_t numQueues =
        _physicalDevice.graphicsQueueFamilyIndex == _physicalDevice.computeQueueFamilyIndex ? 1 : 2;
    const auto extensions = PhysicalDevice::Extensions(deviceExtensions);

    vk::DeviceCreateInfo createInfo(
        {},
        numQueues,
        queueCreateInfo.data(),
        0,
        nullptr,
        extensions.size(),
        extensions.data(),
        &_physicalDevice.features.get<vk::PhysicalDeviceFeatures2>().features,
        _physicalDevice.features.get<vk::PhysicalDeviceVulkan13Features>());

    vk::raii::Device device(*_physicalDevice, createInfo);

    Log::Core::Info("Logical device created");

    vk::raii::Queue graphicsQueue = device.getQueue(_physicalDevice.graphicsQueueFamilyIndex, 0);
    vk::raii::Queue computeQueue  = device.getQueue(_physicalDevice.computeQueueFamilyIndex, 0);

    Log::Core::Info("Device queues created");

    return Device(std::move(device), std::move(graphicsQueue), std::move(computeQueue));
}

} // namespace VoxelDynamics::Vulkan
