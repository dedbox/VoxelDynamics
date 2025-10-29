#include "SDL3/SDL_vulkan.h"

#include "VoxelDynamics/Vulkan/Instance.hpp"

namespace VoxelDynamics::Vulkan
{

Instance::Instance(
    const vk::raii::Context& context, const std::string& appName, const uint64_t appVersion)
    : _instance(CreateInstance(context, appName, appVersion))
{
}

vk::raii::Instance Instance::CreateInstance(
    const vk::raii::Context& context, const std::string& appName, const uint64_t appVersion)
{
    const vk::ApplicationInfo appInfo(
        appName.c_str(), appVersion, EngineName.c_str(), EngineVersion, vk::ApiVersion13);

    const auto layers     = ValidationLayers();
    const auto extensions = RequiredExtensions();

    CheckValidationLayers(layers);
    CheckExtensions(extensions);

    const auto instanceCreateInfo = vk::InstanceCreateInfo({}, &appInfo, layers, extensions);
    if constexpr (is_debugging_enabled)
    {
        const auto debugMessengerCreateInfo = DebugUtilsMessengerCreateInfoEXT();
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
            createInfoChain(instanceCreateInfo, debugMessengerCreateInfo);
        return vk::raii::Instance(context, createInfoChain.get<vk::InstanceCreateInfo>());
    }
    else
        return vk::raii::Instance(context, instanceCreateInfo);
}

constexpr std::vector<const char*> Instance::RequiredExtensions()
{
    uint32_t count      = 0;
    auto raw_extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    std::vector<const char*> extensions(raw_extensions, raw_extensions + count);

    if constexpr (is_debugging_enabled)
        extensions.push_back(vk::EXTDebugUtilsExtensionName);

    return extensions;
}

void Instance::CheckValidationLayers(const std::vector<const char*>& layers)
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

void Instance::CheckExtensions(const std::vector<const char*>& extensions)
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

VKAPI_ATTR vk::Bool32 VKAPI_CALL Instance::DebugUtilsMessengerCallback(
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

constexpr vk::DebugUtilsMessengerCreateInfoEXT Instance::DebugUtilsMessengerCreateInfoEXT()
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

} // namespace VoxelDynamics::Vulkan
