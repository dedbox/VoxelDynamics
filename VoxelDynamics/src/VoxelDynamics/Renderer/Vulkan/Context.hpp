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
    void recreateSwapChain(SDL_Window* window);

    // Pipeline ////////////////////////////////////////////////////////////////////////////////////

    struct Pipeline
    {
        vk::raii::PipelineLayout layout;
        vk::raii::Pipeline graphics;
    };

    Pipeline createGraphicsPipeline(
        const std::string& spvFilePath,
        const vk::VertexInputBindingDescription& vertexBindingDescription,
        std::span<const vk::VertexInputAttributeDescription> vertexAttributeDescriptions) const;

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

    struct VertexBuffer;

    void drawCurrentFrame(
        SDL_Window* window, Frames& frames, Pipeline& pipeline, VertexBuffer& vertexBuffer);
    void recordCommandBuffer(
        Frame& frame, uint32_t imageIndex, Pipeline& pipeline, VertexBuffer& vertexBuffer);

    void transitionImageLayout(
        vk::raii::CommandBuffer& buffer,
        vk::Image& image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask);

    void requestResize();

    // Vertex Buffer ///////////////////////////////////////////////////////////////////////////////

    struct VertexBuffer
    {
        vk::raii::Buffer buffer;
        vk::raii::DeviceMemory memory;
        uint32_t count;

        // dereference operator gives access to the underlying Vulkan object
        vk::raii::Buffer& operator*() { return buffer; }
        const vk::raii::Buffer& operator*() const { return buffer; }
    };

    template <typename R>
        requires std::ranges::contiguous_range<R> && std::ranges::sized_range<R>
    VertexBuffer createVertexBuffer(const R& vertices) const
    {
        using T = std::ranges::range_value_t<R>;

        // create vertex buffer
        vk::BufferCreateInfo createInfo(
            {},
            static_cast<vk::DeviceSize>(std::ranges::size(vertices) * sizeof(T)),
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::SharingMode::eExclusive);

        vk::raii::Buffer buffer(*_device, createInfo);

        setDebugName(
            vk::ObjectType::eBuffer, reinterpret_cast<uint64_t>(&**buffer), "Vertex Buffer");

        // allocate buffer memory
        const auto memReqs = buffer.getMemoryRequirements();
        const auto memType = findMemoryType(
            memReqs.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        vk::MemoryAllocateInfo allocInfo(memReqs.size, memType);

        vk::raii::DeviceMemory memory(*_device, allocInfo);

        setDebugName(
            vk::ObjectType::eDeviceMemory,
            reinterpret_cast<uint64_t>(&**memory),
            "Vertex Buffer Memory");

        // associate this memory with the buffer
        buffer.bindMemory(*memory, 0);

        // copy vertex data to the buffer
        void* data = memory.mapMemory(0, createInfo.size);
        memcpy(data, std::ranges::data(vertices), createInfo.size);
        memory.unmapMemory();

        return VertexBuffer(std::move(buffer), std::move(memory), std::ranges::size(vertices));
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        // query available memory types
        vk::PhysicalDeviceMemoryProperties memProps = _physicalDevice->getMemoryProperties();

        // find a suitable type
        for (const auto& [i, memType] : std::ranges::views::enumerate(memProps.memoryTypes))
            if ((typeFilter * (1U << static_cast<uint32_t>(i))) &&
                (memType.propertyFlags & properties) == properties)
                return i;

        throw std::runtime_error("Could not find a suitable memory type");
    }

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
        bool frameBufferResized = false;

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
    void cleanupSwapChain();
};

} // namespace VoxelDynamics::Vulkan
