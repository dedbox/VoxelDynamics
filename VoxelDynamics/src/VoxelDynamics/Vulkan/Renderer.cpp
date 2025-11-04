#include "VoxelDynamics/Vulkan/Renderer.hpp"

namespace VoxelDynamics::Vulkan
{

Renderer::Renderer(BuildInfo buildInfo_, const Context* context, const Window& window)
    : buildInfo(buildInfo_)
    , _context(context)
    , _cmdBufferManager(_context, buildInfo.maxFramesInFlight)
    , _pipelineManager(_context, vk::raii::PipelineCache(*_context->getDevice(), {}))
    , _swapChain(createSwapChain(window))
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

Buffer Renderer::transferVertexData(const void* data, size_t size) const
{
    return createAndTransferBuffer(
        vk::BufferUsageFlagBits::eVertexBuffer,
        RenderQueue::Graphics,
        vk::PipelineStageFlagBits2::eVertexAttributeInput,
        vk::AccessFlagBits2::eVertexAttributeRead,
        data,
        size,
        "Vertex Buffer",
        "Vertex Buffer Memory");
}

Buffer Renderer::transferIndexData(const void* data, size_t size) const
{
    return createAndTransferBuffer(
        vk::BufferUsageFlagBits::eIndexBuffer,
        RenderQueue::Graphics,
        vk::PipelineStageFlagBits2::eIndexInput,
        vk::AccessFlagBits2::eIndexRead,
        data,
        size,
        "Index Buffer",
        "Index Buffer Memory");
}

void Renderer::drawFrame(
    const Window& window, const vk::raii::Pipeline& pipeline, const CommandRecorder& recorder)
{
    const Device& device       = _context->getDevice();
    const FrameData& frameData = getCurrentFrameData();

    // waith for the current frame to become available
    while (device->waitForFences(
               *frameData.inFlightFence, vk::True, std::numeric_limits<uint64_t>::max()) ==
           vk::Result::eTimeout)
        ;

    // acquire the next swap chain image
    vk::Result result{};
    uint32_t imageIndex{};

    try
    {
        std::tie(result, imageIndex) = _swapChain->acquireNextImage(
            std::numeric_limits<uint64_t>::max(), frameData.imageAvailableSemaphore, nullptr);
    }
    catch (const vk::OutOfDateKHRError& e)
    {
        recreateSwapChain(window);
        return;
    }

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain(window);
        return;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        throw std::runtime_error("Could not acquire the next swap chain image");

    Log::Core::Trace("Acquired swap chain image {}", imageIndex);

    const SwapChainImageData& imageData = _swapChain.images[imageIndex];

    // reset the fence (now that we know we will be submitting work with it)
    device->resetFences(*frameData.inFlightFence);

    // reset the current command pools (and implicitly invalidate their command buffers)
    _cmdBufferManager.resetDynamicBuffers(_swapChain.currentFrame);

    recordCommandBuffer(imageIndex, pipeline, recorder, frameData.cmdBuffer);

    // submit the command buffer to the graphics queue
    vk::PipelineStageFlags waitStages(vk::PipelineStageFlagBits::eColorAttachmentOutput);

    const vk::SubmitInfo submitInfo(
        *frameData.imageAvailableSemaphore,
        waitStages,
        *frameData.cmdBuffer,
        *imageData.renderFinishedSemaphore);
    device.graphicsQeeue.submit(submitInfo, *frameData.inFlightFence);

    // present the current swap chain image after the graphics queue finishes
    try
    {
        const vk::PresentInfoKHR presentInfo(
            *imageData.renderFinishedSemaphore, **_swapChain, imageIndex);
        result = device.graphicsQeeue.presentKHR(presentInfo);
    }
    catch (const vk::OutOfDateKHRError& e)
    {
        recreateSwapChain(window);
    }

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR ||
        _swapChain.resized)
        recreateSwapChain(window);
    else if (result != vk::Result::eSuccess)
        throw std::runtime_error("Could not present swap chain image");

    // move to the next frame
    _swapChain.currentFrame = (_swapChain.currentFrame + 1) % buildInfo.maxFramesInFlight;
}

void Renderer::recordCommandBuffer(
    uint32_t imageIndex,
    const vk::raii::Pipeline& pipeline,
    const CommandRecorder& recorder,
    const vk::raii::CommandBuffer& cmdBuffer)
{
    SwapChainImageData& imageData = _swapChain.images[imageIndex];
    const FrameData& frameData    = getCurrentFrameData();

    cmdBuffer.begin({});

    // transition swap chain image to color attachment layout
    transitionImageLayout(
        cmdBuffer,
        imageData.image,
        vk::ImageLayout::eUndefined,                         // old layout
        vk::ImageLayout::eColorAttachmentOptimal,            // new layout
        {},                                                  // source aspect mask (no need to wait)
        vk::AccessFlagBits2::eColorAttachmentWrite,          // destination aspect mask
        vk::PipelineStageFlagBits2::eTopOfPipe,              // source stage mask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput); // destination stage mask

    // configure the color attachment
    vk::ClearColorValue clearColor = vk::ClearColorValue(
        buildInfo.clearColor[0],
        buildInfo.clearColor[1],
        buildInfo.clearColor[2],
        buildInfo.clearColor[3]);
    vk::RenderingAttachmentInfo attachmentInfo(
        imageData.imageView,                      // image view
        vk::ImageLayout::eColorAttachmentOptimal, // image layout
        vk::ResolveModeFlagBits::eNone,           // resolve mode
        {},                                       // resolve image view
        vk::ImageLayout::eUndefined,              // resolve image layout
        vk::AttachmentLoadOp::eClear,             // load operation
        vk::AttachmentStoreOp::eStore,            // store operation
        clearColor);                              // clear value

    // configure dynamic rendering
    vk::RenderingInfo renderingInfo(
        {},
        vk::Rect2D({0, 0}, _swapChain.extent), // render area
        1,                                     // layer count
        {},                                    // view mask
        attachmentInfo,                        // color attachments
        nullptr,                               // depth attachments
        nullptr);                              // stencil attachments

    // begin rendering
    cmdBuffer.beginRendering(renderingInfo);

    // bind the graphics pipeline
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    // supply dynamic rendering data
    cmdBuffer.setViewport(
        0,
        vk::Viewport(
            0.0F,
            0.0F,
            static_cast<float>(_swapChain.extent.width),
            static_cast<float>(_swapChain.extent.height),
            0.0F,
            1.0F));
    cmdBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swapChain.extent));

    recorder(cmdBuffer);

    // end rendering
    cmdBuffer.endRendering();

    // transition swap chain image to present layout
    transitionImageLayout(
        cmdBuffer,
        imageData.image,
        vk::ImageLayout::eColorAttachmentOptimal,           // old layout
        vk::ImageLayout::ePresentSrcKHR,                    // new layout
        vk::AccessFlagBits2::eColorAttachmentWrite,         // source access mask
        {},                                                 // destination access mask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // source stage
        vk::PipelineStageFlagBits2::eBottomOfPipe);         // destination stage

    cmdBuffer.end();
}

void Renderer::transitionImageLayout(
    const vk::raii::CommandBuffer& buffer,
    vk::Image& image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask)
{
    vk::ImageSubresourceRange subresourceRange(
        vk::ImageAspectFlagBits::eColor, // aspect mask
        0,                               // base mip level
        1,                               // level count
        0,                               // base array layer
        1);                              // layer count

    vk::ImageMemoryBarrier2 barrier(
        srcStageMask,
        srcAccessMask,
        dstStageMask,
        dstAccessMask,
        oldLayout,
        newLayout,
        vk::QueueFamilyIgnored, // source queue family index
        vk::QueueFamilyIgnored, // destination queue family index
        image,
        subresourceRange);

    vk::DependencyInfo dependencyInfo(
        {},
        {},        // memory barrier count
        {},        // memory barriers
        {},        // buffer memory barrier count
        {},        // buffer memory barriers
        1,         // image memory barrier count
        &barrier); // image memory barriers

    buffer.pipelineBarrier2(dependencyInfo);
}

// Swap Chain //////////////////////////////////////////////////////////////////////////////////////

void Renderer::recreateSwapChain(const Window& window)
{
    Log::Core::Info("Recreating swap chain");

    wait();
    cleanupSwapChain();
    _swapChain = createSwapChain(window);
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
        // create command buffer
        vk::raii::CommandBuffer cmdBuffer = _cmdBufferManager.allocatePrimaryBuffer(
            CommandBufferManager::UsageProfile::GraphicsDynamic, i);

        // create "image available" semaphore
        vk::raii::Semaphore imageAvailableSemaphore(*device, semaphoreCreateInfo);

        _context->setDebugName(
            vk::ObjectType::eSemaphore,
            &**imageAvailableSemaphore,
            std::format("Frame Image Available Semaphore {}", i));

        // create frame-in-flight fence
        vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
        vk::raii::Fence fence(*device, fenceCreateInfo);

        _context->setDebugName(
            vk::ObjectType::eFence, &**fence, std::format("Frame In-Flight Fence {}", i));

        // create "transfer finished" semaphore
        vk::raii::Semaphore transferFinishedSemaphore(*device, semaphoreCreateInfo);

        _context->setDebugName(
            vk::ObjectType::eSemaphore,
            &**transferFinishedSemaphore,
            std::format("Frame Transfer Finished Semaphore {}", i));

        frameDatas.emplace_back(
            std::move(cmdBuffer), std::move(imageAvailableSemaphore), std::move(fence));
    }

    Log::Core::Info("Swap chain created:");
    Log::Core::Info("  number of swap chain images: {}", imageCount);
    Log::Core::Info("  surface format: {}", Context::SurfaceFormatName(surfaceFormat));
    Log::Core::Info("  present mode: {}", vk::to_string(presentMode));
    Log::Core::Info("  extent: {}x{}", extent.width, extent.height);

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

Buffer Renderer::createBuffer(
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

    return Buffer(std::move(buffer), std::move(memory));
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

Buffer Renderer::createAndTransferBuffer(
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

    // release ownership of copied data (host)
    const auto destIndex = _cmdBufferManager.getQueueFamilyIndex(destQueue);
    vk::BufferMemoryBarrier2 releaseBarrier(
        vk::PipelineStageFlagBits2::eTransfer, // source stage mask
        vk::AccessFlagBits2::eTransferWrite,   // source access mask
        vk::PipelineStageFlagBits2::eNone,     // destination stage mask
        vk::AccessFlagBits2::eNone,            // destination access mask
        physicalDevice.index->transfer,        // source queue family index
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
    vk::SubmitInfo2 hostSubmitInfo({}, {}, hostCmdBufferSubmitInfo, signalSemaphoreInfo);
    _cmdBufferManager.getQueue(RenderQueue::Transfer).submit2(hostSubmitInfo, nullptr);

    // create a destination command buffer
    auto&& deviceCmdBuffer =
        _cmdBufferManager.allocateOneShotBuffer(destQueue, vk::CommandBufferLevel::ePrimary);

    // begin recording
    deviceCmdBuffer.begin(
        vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // acquire ownership of copied data (device)
    vk::BufferMemoryBarrier2 acquireBarrier(
        vk::PipelineStageFlagBits2::eNone, // source stage mask
        vk::AccessFlagBits2::eNone,        // source access mask
        stage,                             // first stage where the data is used
        access,                            // first access type
        physicalDevice.index->transfer,    // source queue family index
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

    return Buffer(std::move(deviceBuffer), std::move(deviceMemory));
}

} // namespace VoxelDynamics::Vulkan
