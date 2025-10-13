#pragma once

#include "SDL3/SDL_video.h"

#include "vulkan/vulkan_raii.hpp"

namespace VoxelDynamics::Vulkan
{

class Context
{
public:
    struct BuildInfo
    {
        std::string appName;
        uint64_t appVersion;
        vk::PhysicalDeviceType preferredDeviceType;
        int maxFramesInFlight;
    } buildInfo;

    Context(SDL_Window* window, const BuildInfo& buildInfo);

    ~Context() = default;

    // allow moving
    Context(Context&&) noexcept            = default;
    Context& operator=(Context&&) noexcept = default;

    // prevent copying
    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;

    void wait() const;

    // Pipeline ////////////////////////////////////////////////////////////////////////////////////

    struct Pipeline
    {
        vk::raii::PipelineLayout layout;
        vk::raii::Pipeline graphics;
    };

    Pipeline createGraphicsPipeline(const std::string& spvFilePath) const;

    // Frames //////////////////////////////////////////////////////////////////////////////////////

    struct Frame
    {
        vk::raii::CommandPool pool;
        vk::raii::CommandBuffer buffer;
        vk::raii::Semaphore imageAvailableSemaphore;
        vk::raii::Fence inFlightFence;
    };

    struct Frames
    {
        std::vector<Frame> frames;
        size_t currentFrame;
    };

    Frames createFrames() const;
    void destroyFrames(Frames& frames) const;

    void drawCurrentFrame(Frames& frames, Pipeline& pipeline);
    void recordCommandBuffer(Frame& frame, uint32_t imageIndex, Pipeline& pipeline);

    void transitionImageLayout(
        vk::raii::CommandBuffer& buffer,
        vk::Image& image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask);

    ////////////////////////////////////////////////////////////////////////////////////////////////

private:
    using FeaturesChain = vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>;

    vk::raii::Context _context;
    vk::raii::Instance _instance;
    vk::raii::SurfaceKHR _surface;

    struct PhysicalDevice
    {
        vk::raii::PhysicalDevice physicalDevice;
        uint32_t graphicsQueueFamilyIndex;
        uint32_t presentQueueFamilyIndex;
        std::vector<vk::SurfaceFormatKHR> surfaceFormats;
        std::vector<vk::PresentModeKHR> presentModes;
        FeaturesChain features;

        // dereference operator gives access to the underlying Vulkan object
        vk::raii::PhysicalDevice& operator*() { return physicalDevice; }
        const vk::raii::PhysicalDevice& operator*() const { return physicalDevice; }

        // arrow operator gives access to members of the underlying Vulkan object
        vk::raii::PhysicalDevice* operator->() { return &physicalDevice; }
        const vk::raii::PhysicalDevice* operator->() const { return &physicalDevice; }
    } _physicalDevice;

    struct Device
    {
        vk::raii::Device device;
        vk::raii::Queue graphicsQueue;
        vk::raii::Queue presentQueue;

        // dereference operator gives access to the underlying Vulkan object
        vk::raii::Device& operator*() { return device; }
        const vk::raii::Device& operator*() const { return device; }

        // arrow operator gives access to members of the underlying Vulkan object
        vk::raii::Device* operator->() { return &device; }
        const vk::raii::Device* operator->() const { return &device; }
    } _device;

    struct SwapChain
    {
        vk::raii::SwapchainKHR swapChain;
        std::vector<vk::Image> images;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::Extent2D extent;
        std::vector<vk::raii::ImageView> imageViews;
        std::vector<vk::raii::Semaphore> renderFinishedSemaphores;

        // dereference operator gives access to the underlying Vulkan object
        vk::raii::SwapchainKHR& operator*() { return swapChain; }
        const vk::raii::SwapchainKHR& operator*() const { return swapChain; }

        // arrow operator gives access to members of the underlying Vulkan object
        vk::raii::SwapchainKHR* operator->() { return &swapChain; }
        const vk::raii::SwapchainKHR* operator->() const { return &swapChain; }
    } _swapChain;

    // Instance ////////////////////////////////////////////////////////////////////////////////////

    vk::raii::Instance createInstance() const;

    static constexpr std::vector<const char*> InstanceLayers();
    static std::vector<const char*> InstanceExtensions();

    static void CheckInstanceLayers(const std::vector<const char*>& layers);
    static void CheckInstanceExtensions(const std::vector<const char*>& extensions);

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugUtilsMessengerCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
        vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* pUserData);

    static constexpr vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfoEXT();

    // Surface /////////////////////////////////////////////////////////////////////////////////////

    vk::raii::SurfaceKHR createSurface(SDL_Window* window) const;

    // Physical Device /////////////////////////////////////////////////////////////////////////////

    PhysicalDevice pickPhysicalDevice() const;

    static constexpr std::vector<const char*> DeviceExtensions();

    static bool CheckDeviceExtensions(const std::vector<std::string>&, const size_t i);

    static std::string SurfaceFormatName(const vk::SurfaceFormatKHR& format);
    static std::string SurfaceFormatNames(const std::vector<vk::SurfaceFormatKHR>& formats);
    static std::string PresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes);

    static std::optional<FeaturesChain> CreateFeaturesChain(
        const vk::raii::PhysicalDevice& physicalDevice, const size_t i);

    // Logical Device //////////////////////////////////////////////////////////////////////////////

    Device createDevice() const;

    void setDebugName(vk::ObjectType type, uint64_t handle, const std::string& name) const
    {
        _device->setDebugUtilsObjectNameEXT(
            vk::DebugUtilsObjectNameInfoEXT(type, handle, name.c_str()));
    }

    // Swap Chain //////////////////////////////////////////////////////////////////////////////////

    SwapChain createSwapChain(SDL_Window* window) const;
};

} // namespace VoxelDynamics::Vulkan
