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

        // forward subscript operator to the underlying vector
        Frame& operator[](const size_t i) { return frames[i]; }
        const Frame& operator[](const size_t i) const { return frames[i]; }
    };

    Frames createFrames() const;
    void destroyFrames(Frames& frames) const;

    struct VertexBuffer;
    struct IndexBuffer;

    void drawCurrentFrame(
        SDL_Window* window,
        Frames& frames,
        Pipeline& pipeline,
        VertexBuffer& vertexBuffer,
        std::optional<std::reference_wrapper<IndexBuffer>> indexBuffer = std::nullopt);

    void recordCommandBuffer(
        Frame& frame,
        uint32_t imageIndex,
        Pipeline& pipeline,
        VertexBuffer& vertexBuffer,
        std::optional<std::reference_wrapper<IndexBuffer>> indexBuffer = std::nullopt);

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

    // Command Buffer //////////////////////////////////////////////////////////////////////////////

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        const std::string& bufferName,
        const std::string& memoryName) const;

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

    template <typename T>
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createStagingBuffer(
        const std::vector<T>& data,
        const vk::DeviceSize size,
        const std::string& bufferName,
        const std::string& memoryName) const
    {

        auto&& [buffer, memory] = createBuffer(
            size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            bufferName,
            memoryName);

        // copy vertex data to the staging buffer
        void* bufferData = memory.mapMemory(0, size);
        memcpy(bufferData, std::ranges::data(data), size);
        memory.unmapMemory();

        return std::make_pair(std::move(buffer), std::move(memory));
    }

    [[nodiscard]] std::pair<vk::raii::CommandBuffer, vk::raii::Semaphore> transferStagingBufferOut(
        const Frame& frame,
        const vk::raii::Buffer& stagingBuffer,
        const vk::raii::Buffer& targetBuffer,
        const vk::DeviceSize size) const;

    void transferStagingBufferIn(
        const Frame& frame,
        const vk::raii::Buffer& targetBuffer,
        const vk::PipelineStageFlags2 stage,
        const vk::AccessFlagBits2 access,
        [[maybe_unused]] vk::raii::CommandBuffer&& outCmdBuffer,
        vk::raii::Semaphore&& semaphore) const;

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

    template <typename T>
    VertexBuffer createVertexBuffer(Frames& frames, const std::vector<T>& vertices) const
    {
        const auto size   = static_cast<vk::DeviceSize>(vertices.size() * sizeof(T));
        const auto& frame = frames[frames.currentFrame];

        // create a staging buffer
        auto&& [stagingBuffer, stagingMemory] = createStagingBuffer(
            vertices, size, "Vertex Staging Buffer", "Vertex Staging Buffer Memory");

        // create a vertex buffer
        auto&& [vertexBuffer, vertexMemory] = createBuffer(
            size,
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            "Vertex Buffer",
            "Vertex Buffer Memory");

        // perform the transfer
        auto&& [outCmdBuffer, semaphore] =
            transferStagingBufferOut(frame, stagingBuffer, vertexBuffer, size);
        transferStagingBufferIn(
            frame,
            vertexBuffer,
            vk::PipelineStageFlagBits2::eVertexAttributeInput,
            vk::AccessFlagBits2::eVertexAttributeRead,
            std::move(outCmdBuffer),
            std::move(semaphore));

        return VertexBuffer(std::move(vertexBuffer), std::move(vertexMemory), vertices.size());
    }

    // Index Buffer ////////////////////////////////////////////////////////////////////////////////

    struct IndexBuffer
    {
        vk::raii::Buffer buffer;
        vk::raii::DeviceMemory memory;
        uint32_t count;

        // dereference operator gives access to the underlying Vulkan object
        vk::raii::Buffer& operator*() { return buffer; }
        const vk::raii::Buffer& operator*() const { return buffer; }
    };

    template <typename T>
    IndexBuffer createIndexBuffer(Frames& frames, const std::vector<T>& indices) const
    {
        const auto size   = static_cast<vk::DeviceSize>(indices.size() * sizeof(T));
        const auto& frame = frames[frames.currentFrame];

        // create a staging buffer
        auto&& [stagingBuffer, stagingMemory] = createStagingBuffer(
            indices, size, "Index Staging Buffer", "Index Staging Buffer Memory");

        // create a vertex buffer
        auto&& [indexBuffer, indexMemory] = createBuffer(
            size,
            vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            "Index Buffer",
            "Imdex Buffer Memory");

        // perform the transfer
        auto&& [outCmdBuffer, semaphore] =
            transferStagingBufferOut(frame, stagingBuffer, indexBuffer, size);
        transferStagingBufferIn(
            frame,
            indexBuffer,
            vk::PipelineStageFlagBits2::eIndexInput,
            vk::AccessFlagBits2::eIndexRead,
            std::move(outCmdBuffer),
            std::move(semaphore));

        return IndexBuffer(std::move(indexBuffer), std::move(indexMemory), indices.size());
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
