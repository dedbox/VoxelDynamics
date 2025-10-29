#pragma once

#include "vulkan/vulkan_raii.hpp"

namespace VoxelDynamics::Vulkan
{

class Instance
{
public:
    Instance(
        const vk::raii::Context& context, const std::string& appName, const uint64_t appVersion);

    ~Instance() = default;

    // allow move
    Instance(Instance&&) noexcept            = default;
    Instance& operator=(Instance&&) noexcept = default;

    // prevent copy
    Instance(const Instance&)            = delete;
    Instance& operator=(const Instance&) = delete;

    // proxy dereference operator
    vk::raii::Instance& operator*() { return _instance; }
    const vk::raii::Instance& operator*() const { return _instance; }

    // proxy arrow operator
    vk::raii::Instance* operator->() { return &_instance; }
    const vk::raii::Instance* operator->() const { return &_instance; }

private:
    vk::raii::Instance _instance;

    static vk::raii::Instance CreateInstance(
        const vk::raii::Context& context, const std::string& appName, const uint64_t appVersion);

    static constexpr std::vector<const char*> ValidationLayers()
    {
        if constexpr (is_debugging_enabled)
            return {
                // "VK_LAYER_LUNARG_api_dump",
                "VK_LAYER_KHRONOS_validation",
            };
        else
            return {};
    }

    static constexpr std::vector<const char*> RequiredExtensions();

    static void CheckValidationLayers(const std::vector<const char*>& layers);
    static void CheckExtensions(const std::vector<const char*>& extensions);

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugUtilsMessengerCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
        vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* pUserData);

    static constexpr vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfoEXT();
};

} // namespace VoxelDynamics::Vulkan
