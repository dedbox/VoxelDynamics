#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace VoxelDynamics::Vulkan
{

class Instance
{
public:
    Instance(
        const vk::raii::Context& context, const std::string& appName, const uint32_t appVersion);

    vk::raii::Instance& operator*() { return _instance; }

private:
    vk::raii::Instance _instance;

    static vk::raii::Instance CreateInstance(
        const vk::raii::Context& context, const std::string& appName, const uint32_t appVersion);

    static std::vector<const char*> ValidationLayers();
    static std::vector<const char*> Extensions();

    static void CheckValidationLayers(const std::vector<const char*>& layers);
    static void CheckExtensions(const std::vector<const char*>& extensions);

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugUtilsMessengerCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
        vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* /*pUserData*/);

    static vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfoEXT();
};

} // namespace VoxelDynamics::Vulkan
