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

void Renderer::wait() const
{
    _context->getDevice()->waitIdle();
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

} // namespace VoxelDynamics::Vulkan
