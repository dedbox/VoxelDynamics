#include "VoxelDynamics/Vulkan/Renderer.hpp"

namespace VoxelDynamics::Vulkan
{

Renderer::Renderer(BuildInfo buildInfo_, const Context* context, const Window& window)
    : buildInfo(buildInfo_)
    , _context(context)
    , _swapChain(createSwapChain(window))
    , _cmdBufferManager(_context, buildInfo.maxFramesInFlight)
    , _pipelineManager(_context, vk::raii::PipelineCache(*_context->getDevice(), {}))
{
    if (!_context)
        throw std::invalid_argument("Context pointer cannot be null");

    Log::Core::Info("Vulkan renderer initialized");
}

void Renderer::resetOneShotBuffers()
{
    _cmdBufferManager.resetOneShotBuffers();
}

void Renderer::wait() const
{
    _context->getDevice()->waitIdle();
}

void Renderer::transferVertexData(const void* data, size_t size) const
{
    createAndTransferBuffer(
        vk::BufferUsageFlagBits::eVertexBuffer,
        RenderQueue::Graphics,
        vk::PipelineStageFlagBits2::eVertexAttributeInput,
        vk::AccessFlagBits2::eVertexAttributeRead,
        data,
        size,
        "Vertex Buffer",
        "Vertex Buffer Memory");
}

void Renderer::transferIndexData(const void* data, size_t size) const
{
    createAndTransferBuffer(
        vk::BufferUsageFlagBits::eIndexBuffer,
        RenderQueue::Graphics,
        vk::PipelineStageFlagBits2::eIndexInput,
        vk::AccessFlagBits2::eIndexRead,
        data,
        size,
        "Index Buffer",
        "Index Buffer Memory");
}

// Swap Chain //////////////////////////////////////////////////////////////////////////////////////

void Renderer::recreateSwapChain(const Window& window)
{
    wait();
    cleanupSwapChain();
    createSwapChain(window);
}

SwapChain Renderer::createSwapChain(const Window& window) const
{
    Log::Core::Info("Querying surface capabilities:");

    const auto& physicalDevice = _context->getPhysicalDevice();
    const auto& surface        = _context->getSurface();
    const auto& device         = _context->getDevice();

    const auto caps = physicalDevice->getSurfaceCapabilitiesKHR(surface);

    // determine swap chain image dimensions
    const auto extent = [&]() -> vk::Extent2D {
        if (caps.currentExtent.width != 0xFFFFFFFF)
            return caps.currentExtent;

        // TODO do we need to handle window minimize separately?
        const auto& [width, height] = window.getSize();

        return {
            std::clamp<uint32_t>(width, caps.minImageExtent.width, caps.minImageExtent.height),
            std::clamp<uint32_t>(height, caps.minImageExtent.height, caps.minImageExtent.height),
        };
    }();

    Log::Core::Info("  extent: {}x{}", extent.width, extent.height);

    // choose a surface format
    const auto surfaceFormat = [&]() -> vk::SurfaceFormatKHR {
        // check if device prefers BGR formats
        const auto isNativeBgr = [&]() -> bool {
            for (const auto format : physicalDevice.surfaceFormats)
            {
                switch (format.format)
                {
                case vk::Format::eR8G8B8A8Unorm:
                case vk::Format::eR8G8B8A8Srgb:
                case vk::Format::eA2R10G10B10UnormPack32:
                    return false;
                case vk::Format::eB8G8R8A8Unorm:
                case vk::Format::eB8G8R8A8Srgb:
                case vk::Format::eA2B10G10R10UnormPack32:
                    return true;
                default:
                    break;
                }
            }
            return false;
        }();

        Log::Core::Info("  native BGR: {}", isNativeBgr ? "yes" : "no");

        // TODO determine client's preferred format and color space
        const auto preferred = vk::SurfaceFormatKHR(
            isNativeBgr ? vk::Format::eB8G8R8A8Unorm : vk::Format::eR8G8B8A8Unorm,
            vk::ColorSpaceKHR::eSrgbNonlinear);

        Log::Core::Info(
            "  preferred format: {} / {}",
            vk::to_string(preferred.format),
            vk::to_string(preferred.colorSpace));

        Log::Core::Info("  available formats:");
        for (const auto& format : physicalDevice.surfaceFormats)
            Log::Core::Info(
                "    {} / {}", vk::to_string(format.format), vk::to_string(format.colorSpace));

        // check if device supports client's preferred format and color space
        for (const auto& format : physicalDevice.surfaceFormats)
            if (format.format == preferred.format && format.colorSpace == preferred.colorSpace)
                return format;

        Log::Core::Trace("Could not find the preferred swap chain format and color space");

        // if not, check if device supports client's preferred format with any color space
        for (const auto& format : physicalDevice.surfaceFormats)
            if (format.format == preferred.format)
                return format;

        Log::Core::Trace("Could not find the preferred swap chain format");

        // Otherwise, default to the first format and color space
        return physicalDevice.surfaceFormats[0];
    }();

    // choose a present mode
    const auto presentMode = [&]() -> vk::PresentModeKHR {
        const auto& modes = physicalDevice.presentModes;
        if (std::ranges::find(modes, vk::PresentModeKHR::eMailbox) != modes.end())
            return vk::PresentModeKHR::eMailbox;
        if (std::ranges::find(modes, vk::PresentModeKHR::eImmediate) != modes.end())
            return vk::PresentModeKHR::eImmediate;
        if (std::ranges::find(modes, vk::PresentModeKHR::eFifo) != modes.end())
            return vk::PresentModeKHR::eFifo;
        throw std::runtime_error("Device offers no supported present modes");
    }();

    // determine the number of swap chain images
    const auto imageCount = caps.maxImageCount == 0
                                ? caps.minImageCount + 1
                                : std::min(caps.minImageCount + 1, caps.maxImageCount);

    // create the swap chain
    const auto createInfo = vk::SwapchainCreateInfoKHR(
        {},
        surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        physicalDevice.uniqueIndices,
        caps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity
            ? vk::SurfaceTransformFlagBitsKHR::eIdentity
            : caps.currentTransform,
        caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque
            ? vk::CompositeAlphaFlagBitsKHR::eOpaque
            : vk::CompositeAlphaFlagBitsKHR::eInherit,
        presentMode);

    auto swapChain = vk::raii::SwapchainKHR(*device, createInfo);
    auto images    = swapChain.getImages();

    _context->setDebugName(vk::ObjectType::eSwapchainKHR, &**swapChain, "Vulkan Swap Chain");

    // create an image data object for each image in the swap chain
    std::vector<SwapChainImageData> imageDatas;
    imageDatas.reserve(images.size());

    vk::SemaphoreCreateInfo semaphoreCreateInfo{};

    for (const auto& [i, image] : std::ranges::views::enumerate(images))
    {
        _context->setDebugName(
            vk::ObjectType::eImage, &*image, std::format("Swap Chain Image {}", i));

        // create image view
        const vk::ImageViewCreateInfo imageViewCreateInfo(
            {},
            image,
            vk::ImageViewType::e2D,
            surfaceFormat.format,
            {},
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        vk::raii::ImageView imageView(*device, imageViewCreateInfo);

        _context->setDebugName(
            vk::ObjectType::eImageView, &**imageView, std::format("Swap Chain Image View {}", i));

        // create "render finished" semaphore
        vk::raii::Semaphore semaphore(*device, semaphoreCreateInfo);

        _context->setDebugName(
            vk::ObjectType::eSemaphore,
            static_cast<VkSemaphore>(*semaphore),
            std::format("Swap Chain Render Finished Semaphore {}", i));

        imageDatas.emplace_back(image, std::move(imageView), std::move(semaphore));
    }

    // create a frame data object for each frame-in-flight
    std::vector<FrameData> frameDatas;
    frameDatas.reserve(buildInfo.maxFramesInFlight);

    for (const auto i : std::ranges::views::iota(0U, buildInfo.maxFramesInFlight))
    {
        // create "image available" semaphore
        vk::raii::Semaphore semaphore(*device, semaphoreCreateInfo);

        _context->setDebugName(
            vk::ObjectType::eSemaphore,
            &**semaphore,
            std::format("Frame Image Available Semaphore {}", i));

        // create frame-in-flight fence
        vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
        vk::raii::Fence fence(*device, fenceCreateInfo);

        _context->setDebugName(
            vk::ObjectType::eFence, &**fence, std::format("Frame In-Flight Fence {}", i));

        frameDatas.emplace_back(std::move(semaphore), std::move(fence));
    }

    Log::Core::Info("Swap chain created:");
    Log::Core::Info("  number of swap chain images: {}", imageCount);
    Log::Core::Info("  chosen format: {}", Context::SurfaceFormatName(surfaceFormat));
    Log::Core::Info("  present mode: {}", vk::to_string(presentMode));

    return SwapChain(
        std::move(swapChain),
        surfaceFormat,
        extent,
        false,
        std::move(imageDatas),
        std::move(frameDatas),
        0);
}

void Renderer::cleanupSwapChain()
{

    _swapChain.swapChain = VK_NULL_HANDLE;
    _swapChain.images.clear();
    _swapChain.frames.clear();
}

const FrameData& Renderer::getCurrentFrameData() const
{
    return _swapChain.frames[_swapChain.currentFrame];
}

// Command Buffers /////////////////////////////////////////////////////////////////////////////////

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Renderer::createBuffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    const std::string& bufferName,
    const std::string& memoryName) const
{
    const auto& device = _context->getDevice();

    // create buffer handle
    vk::BufferCreateInfo createInfo({}, size, usage, vk::SharingMode::eExclusive);
    vk::raii::Buffer buffer(*device, createInfo);

    _context->setDebugName(vk::ObjectType::eBuffer, &**buffer, bufferName);

    // allocate buffer memory
    const auto memReqs = buffer.getMemoryRequirements();
    const auto memType = findMemoryType(memReqs.memoryTypeBits, properties);

    vk::MemoryAllocateInfo allocInfo(memReqs.size, memType);
    vk::raii::DeviceMemory memory(*device, allocInfo);

    _context->setDebugName(vk::ObjectType::eDeviceMemory, &**memory, memoryName);

    // associate the memory with the handle
    buffer.bindMemory(*memory, 0);

    return std::make_pair(std::move(buffer), std::move(memory));
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
    // query available memory types
    vk::PhysicalDeviceMemoryProperties memProps =
        _context->getPhysicalDevice()->getMemoryProperties();

    // find a suitable type
    for (const auto& [i, memType] : std::ranges::views::enumerate(memProps.memoryTypes))
        if ((typeFilter & (1U << static_cast<uint32_t>(i))) &&
            (memType.propertyFlags & properties) == properties)
            return i;

    throw std::runtime_error("Could not find a suitable memory type");
}

void Renderer::createAndTransferBuffer(
    vk::BufferUsageFlagBits usage,
    RenderQueue destQueue,
    const vk::PipelineStageFlagBits2 stage,
    const vk::AccessFlagBits2 access,
    const void* data,
    size_t size,
    const std::string& bufferName,
    const std::string& memoryName) const
{
    const auto& device         = _context->getDevice();
    const auto& physicalDevice = _context->getPhysicalDevice();

    // allocate a host buffer
    auto&& [hostBuffer, hostMemory] = createBuffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        std::format("Host {}", bufferName),
        std::format("Host {}", memoryName));

    // copy data to the host buffer
    void* bufferData = hostMemory.mapMemory(0, size);
    memcpy(bufferData, data, size);
    hostMemory.unmapMemory();

    // allocate a device-local buffer
    auto&& [deviceBuffer, deviceMemory] = createBuffer(
        size,
        usage | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        std::format("Device {}", bufferName),
        std::format("Device {}", memoryName));

    // create a transfer command buffer
    auto&& hostCmdBuffer = _cmdBufferManager.allocateOneShotBuffer(
        RenderQueue::Transfer, vk::CommandBufferLevel::ePrimary);

    // begin recording
    hostCmdBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // copy host buffer to device-local buffer
    hostCmdBuffer.copyBuffer(*hostBuffer, *deviceBuffer, vk::BufferCopy(0, 0, size));

    // host releases ownership of copied data
    const auto destIndex = _cmdBufferManager.getQueueFamilyIndex(destQueue);
    vk::BufferMemoryBarrier2 releaseBarrier(
        vk::PipelineStageFlagBits2::eTransfer, // source stage mask
        vk::AccessFlagBits2::eTransferWrite,   // source access mask
        vk::PipelineStageFlagBits2::eNone,     // destination stage mask
        vk::AccessFlagBits2::eNone,            // destination access mask
        physicalDevice.transferIndex,          // source queue family index
        destIndex,                             // destination queue family index
        deviceBuffer,                          // the resource being transferred
        0,                                     // offset
        vk::WholeSize);                        // size

    vk::DependencyInfo releaseDepInfo({}, nullptr, releaseBarrier, nullptr);
    hostCmdBuffer.pipelineBarrier2(releaseDepInfo);

    // end recording
    hostCmdBuffer.end();

    // create an acquired-released semaphore
    vk::raii::Semaphore semaphore(*device, vk::SemaphoreCreateInfo());

    _context->setDebugName(vk::ObjectType::eSemaphore, &**semaphore, "One-Shot Transfer Semaphore");

    // signal the semaphore after releasing
    vk::SemaphoreSubmitInfo signalSemaphoreInfo(
        *semaphore, {}, vk::PipelineStageFlagBits2::eTransfer);

    // submit the command buffer
    vk::CommandBufferSubmitInfo hostCmdBufferSubmitInfo(hostCmdBuffer);
    vk::SubmitInfo2 hostHubmitInfo({}, {}, hostCmdBufferSubmitInfo, signalSemaphoreInfo);
    _cmdBufferManager.getQueue(RenderQueue::Transfer).submit2(hostHubmitInfo);

    // create a destination command buffer
    auto&& deviceCmdBuffer =
        _cmdBufferManager.allocateOneShotBuffer(destQueue, vk::CommandBufferLevel::ePrimary);

    // begin recording
    deviceCmdBuffer.begin(
        vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // device acquires ownership of copied data
    vk::BufferMemoryBarrier2 acquireBarrier(
        vk::PipelineStageFlagBits2::eNone, // source stage mask
        vk::AccessFlagBits2::eNone,        // source access mask
        stage,                             // first stage where the data is used
        access,                            // first access type
        physicalDevice.transferIndex,      // source queue family index
        destIndex,                         // destination queue family index
        deviceBuffer,                      // the resource being transferred
        0,                                 // offset
        vk::WholeSize);                    // size

    vk::DependencyInfo acquireDepInfo({}, nullptr, acquireBarrier, nullptr);
    deviceCmdBuffer.pipelineBarrier2(acquireDepInfo);

    // end recording
    deviceCmdBuffer.end();

    // wait on semaphore before acquiring
    vk::SemaphoreSubmitInfo waitSemaphoreInfo(*std::move(semaphore), {}, stage);

    // create an end-of-submission fence
    vk::raii::Fence fence(*device, vk::FenceCreateInfo());

    // submit the command buffer
    vk::CommandBufferSubmitInfo deviceCmdBufferSubmitInfo(deviceCmdBuffer);
    vk::SubmitInfo2 deviceSubmitInfo({}, waitSemaphoreInfo, deviceCmdBufferSubmitInfo, {});
    _cmdBufferManager.getQueue(destQueue).submit2(deviceSubmitInfo, *fence);

    // wait for end of sumission
    if (device->waitForFences(*fence, vk::True, std::numeric_limits<uint64_t>::max()) !=
        vk::Result::eSuccess)
        throw std::runtime_error("Could not wait for fence");

    device->resetFences(*fence);

    Log::Core::Info("Uploaded {} bytes ({}) to {} queue", size, bufferName, toString(destQueue));
}

} // namespace VoxelDynamics::Vulkan
