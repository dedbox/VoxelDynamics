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
        vk::raii::CommandPool gpPool;
        vk::raii::CommandBuffer gpBuffer;
        vk::raii::CommandPool transferPool;
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

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        const std::string& bufferName,
        const std::string& memoryName) const;

    template <typename T>
    VertexBuffer createVertexBuffer(const std::vector<T>& vertices) const
    {
        // create vertex buffer
        const auto size         = static_cast<vk::DeviceSize>(vertices.size() * sizeof(T));
        auto&& [buffer, memory] = createBuffer(
            size,
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            "Vertex Buffer",
            "Vertex Buffer Memory");

        // copy vertex data to the buffer
        void* data = memory.mapMemory(0, size);
        memcpy(data, std::ranges::data(vertices), size);
        memory.unmapMemory();

        return VertexBuffer(std::move(buffer), std::move(memory), vertices.size());
    }

    template <typename T>
    VertexBuffer createStagedVertexBuffer(Frames& frames, const std::vector<T>& vertices) const
    {
        const auto size = static_cast<vk::DeviceSize>(vertices.size() * sizeof(T));

        // create staging buffer
        auto&& [stagingBuffer, stagingMemory] = createBuffer(
            size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            "Vertex Staging Buffer",
            "Vertex Staging Buffer Memory");

        // copy vertex data to the staging buffer
        void* data = stagingMemory.mapMemory(0, size);
        memcpy(data, std::ranges::data(vertices), size);
        stagingMemory.unmapMemory();

        // create vertex buffer
        auto&& [vertexBuffer, vertexMemory] = createBuffer(
            size,
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            "Vertex Staging Buffer",
            "Vertex Staging Buffer Memory");

        // create transfer semaphore
        Frame& frame = frames.frames[frames.currentFrame];
        vk::raii::Semaphore semaphore(*_device, vk::SemaphoreCreateInfo());

        // record transfer command buffer
        vk::CommandBufferAllocateInfo transferAllocateInfo(
            *frame.transferPool, vk::CommandBufferLevel::ePrimary, 1);
        auto transferBuffers = _device->allocateCommandBuffers(transferAllocateInfo);
        vk::raii::CommandBuffer transferBuffer = std::move(transferBuffers.front());

        setDebugName(
            vk::ObjectType::eCommandBuffer,
            reinterpret_cast<uint64_t>(&**transferBuffer),
            std::format("Staged Transfer Command Buffer"));

        transferBuffer.begin(
            vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        // copy staging buffer to vertex buffer
        transferBuffer.copyBuffer(*stagingBuffer, *vertexBuffer, vk::BufferCopy(0, 0, size));

        // "release ownership" barrier
        vk::BufferMemoryBarrier2 releaseBarrier(
            vk::PipelineStageFlagBits2::eTransfer,    // source stage mask
            vk::AccessFlagBits2::eTransferWrite,      // source access mask
            vk::PipelineStageFlagBits2::eNone,        // destination stage mask
            vk::AccessFlagBits2::eNone,               // destination access mask
            _physicalDevice.transferQueueFamilyIndex, // source queue family index
            _physicalDevice.graphicsQueueFamilyIndex, // destination queue family index
            vertexBuffer,                             // the resource being transferred
            0,                                        // offset
            vk::WholeSize);                           // size
        vk::DependencyInfo releaseDepInfo({}, nullptr, releaseBarrier, nullptr);
        transferBuffer.pipelineBarrier2(releaseDepInfo);

        transferBuffer.end();

        // submit transfer command buffer and signal semaphore
        vk::SemaphoreSubmitInfo signalSemaphoreInfo(
            *semaphore, {}, vk::PipelineStageFlagBits2::eTransfer);
        vk::CommandBufferSubmitInfo transferCmdBufferInfo(transferBuffer);
        vk::SubmitInfo2 transferSubmitInfo(
            {}, 0, nullptr, 1, &transferCmdBufferInfo, 1, &signalSemaphoreInfo);

        _device.transferQueue.submit2(transferSubmitInfo, nullptr);

        // record graphics command buffer
        vk::CommandBufferAllocateInfo graphicsAllocateInfo(
            *frame.gpPool, vk::CommandBufferLevel::ePrimary, 1);
        auto graphicsBuffers = _device->allocateCommandBuffers(graphicsAllocateInfo);
        vk::raii::CommandBuffer graphicsBuffer = std::move(graphicsBuffers.front());

        setDebugName(
            vk::ObjectType::eCommandBuffer,
            reinterpret_cast<uint64_t>(&**graphicsBuffer),
            std::format("Staged Graphics Command Buffer"));

        graphicsBuffer.begin(
            vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        // "acquire ownership" barrier
        vk::BufferMemoryBarrier2 acquirearrier(
            vk::PipelineStageFlagBits2::eNone,                 // source stage mask
            vk::AccessFlagBits2::eNone,                        // source access mask
            vk::PipelineStageFlagBits2::eVertexAttributeInput, // first stage where data is used
            vk::AccessFlagBits2::eVertexAttributeRead,         // first access type
            _physicalDevice.transferQueueFamilyIndex,          // source queue family index
            _physicalDevice.graphicsQueueFamilyIndex,          // destination queue family index
            vertexBuffer,                                      // the resource being transferred
            0,                                                 // offset
            vk::WholeSize);                                    // size
        vk::DependencyInfo acquireDepInfo({}, nullptr, acquirearrier, nullptr);
        graphicsBuffer.pipelineBarrier2(acquireDepInfo);

        graphicsBuffer.end();

        // submit graphics command buffer, waiting for the semaphore
        vk::SemaphoreSubmitInfo waitSemaphoreInfo(
            *semaphore, {}, vk::PipelineStageFlagBits2::eVertexAttributeInput);
        vk::CommandBufferSubmitInfo graphicsCmdBufferInfo(graphicsBuffer);
        vk::SubmitInfo2 graphicsSubmitInfo({}, 1, &waitSemaphoreInfo, 1, &graphicsCmdBufferInfo);

        vk::raii::Fence fence(*_device, vk::FenceCreateInfo());

        _device.graphicsQueue.submit2(graphicsSubmitInfo, *fence);

        if (_device->waitForFences(*fence, vk::True, std::numeric_limits<uint64_t>::max()) !=
            vk::Result::eSuccess)
            throw std::runtime_error("Could not wait for fence");

        _device->resetFences(*fence);

        return VertexBuffer(std::move(vertexBuffer), std::move(vertexMemory), vertices.size());
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

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
        uint32_t transferQueueFamilyIndex;
        std::vector<uint32_t> uniqueQueueFamilyIndices;
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
        vk::raii::Queue transferQueue;

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
