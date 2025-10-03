#include "VoxelDynamics/Renderer/Vulkan/SwapChain.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace VoxelDynamics::Vulkan
{

SwapChain::SwapChain(
    vk::raii::SwapchainKHR handle_,
    vk::SurfaceFormatKHR surfaceFormat_,
    ColorSpace colorSpace_,
    std::vector<Image> images_,
    std::vector<vk::Semaphore> acquireSemaphores_)
    : handle(std::move(handle_))
    , surfaceFormat(surfaceFormat_)
    , colorSpace(colorSpace_)
    , images(std::move(images_))
    , acquireSemaphores(std::move(acquireSemaphores_))
{
}

SwapChain SwapChain::Create(
    const PhysicalDevice& physicalDevice,
    const Device& device,
    uint32_t width,
    uint32_t height,
    ColorSpace requestedColorSpace)
{
    Log::Core::Assert(physicalDevice.surface.handle != VK_NULL_HANDLE, "OS surface is empty");

    const auto caps =
        physicalDevice.handle.getSurfaceCapabilitiesKHR(physicalDevice.surface.handle);
    const auto surfaceFormat = ChooseSurfaceFormat(physicalDevice, requestedColorSpace);
    const auto props         = physicalDevice.handle.getFormatProperties(surfaceFormat.format);
    const auto modes         = physicalDevice.surface.presentModes;

    const uint32_t imageCount = [&]() {
        if (caps.maxImageCount == 0)
            return caps.minImageCount + 1;
        else
            return std::min(caps.minImageCount + 1, caps.maxImageCount);
    }();

    const auto presentMode = [&]() {
        // TODO make first check linux-only?
        if (std::ranges::find(modes, vk::PresentModeKHR::eImmediate) != modes.end())
            return vk::PresentModeKHR::eImmediate;
        if (std::ranges::find(modes, vk::PresentModeKHR::eMailbox) != modes.end())
            return vk::PresentModeKHR::eMailbox;
        return vk::PresentModeKHR::eFifo;
    }();

    const auto usageFlags = [&]() {
        auto usageFlags = vk::ImageUsageFlagBits::eColorAttachment |
                          vk::ImageUsageFlagBits::eTransferDst |
                          vk::ImageUsageFlagBits::eTransferSrc;

        if ((caps.supportedUsageFlags & vk::ImageUsageFlagBits::eStorage) &&
            (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eStorageImage))
            usageFlags |= vk::ImageUsageFlagBits::eStorage;

        return usageFlags;
    }();

    const vk::SwapchainCreateInfoKHR createInfo(
        {},
        physicalDevice.surface.handle,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        caps.currentExtent.width == std::numeric_limits<uint32_t>::max()
            ? vk::Extent2D(width, height)
            : caps.currentExtent,
        1,
        usageFlags,
        vk::SharingMode::eExclusive,
        1,
        &physicalDevice.graphicsQueueFamilyIndex,
        caps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity
            ? vk::SurfaceTransformFlagBitsKHR::eIdentity
            : caps.currentTransform,
        caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque
            ? vk::CompositeAlphaFlagBitsKHR::eOpaque
            : vk::CompositeAlphaFlagBitsKHR::eInherit,
        presentMode);

    vk::raii::SwapchainKHR swapChain = device.handle.createSwapchainKHR(createInfo);

    const auto vk_images = swapChain.getImages();

    std::vector<Image> images;
    images.reserve(vk_images.size());

    std::vector<vk::Semaphore> acquireSemaphores;
    acquireSemaphores.reserve(vk_images.size());

    for (const auto& [i, image] : std::ranges::views::enumerate(vk_images))
    {
        device.setDebugName(
            vk::ObjectType::eImage,
            reinterpret_cast<uint64_t>(&*image), // NOLINT
            std::format("SwapChain Image {}", i));

        const vk::ImageViewCreateInfo imageViewCreateInfo(
            {},
            image,
            vk::ImageViewType::e2D,
            surfaceFormat.format,
            {},
            vk::ImageSubresourceRange(
                vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1));

        images.emplace_back(
            image, ImageType::SwapChain, device.handle.createImageView(imageViewCreateInfo));

        vk::SemaphoreCreateFlags flags = {};
        vk::SemaphoreCreateInfo semaphoreCreateInfo(flags);

        acquireSemaphores.push_back(device.handle.createSemaphore(semaphoreCreateInfo));
    }

    Log::Core::Info("Created swap chain:");
    Log::Core::Info("  image count: {}", imageCount);
    Log::Core::Info("  format: {}", vk::to_string(surfaceFormat.format));
    Log::Core::Info("  color space: {}", vk::to_string(surfaceFormat.colorSpace));
    Log::Core::Info("  present mode: {}", vk::to_string(presentMode));
    Log::Core::Info("  usage flags: {}", vk::to_string(usageFlags));

    return SwapChain(
        std::move(swapChain),
        surfaceFormat,
        requestedColorSpace,
        std::move(images),
        std::move(acquireSemaphores));
}

void SwapChain::destroy()
{
    images.clear();
    auto sc = std::make_unique<vk::raii::SwapchainKHR>(std::move(handle));
}

vk::SurfaceFormatKHR SwapChain::ChooseSurfaceFormat(
    const PhysicalDevice& physicalDevice, ColorSpace requestedColorSpace)
{
    Log::Core::Trace("Requested color space: {}", vk::to_string(to_vk(requestedColorSpace)));

    const auto isNativeBGR = [&]() {
        for (const vk::SurfaceFormatKHR& fmt : physicalDevice.surface.formats)
        {
            switch (fmt.format)
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

    Log::Core::Trace("Native BGR support: {}", isNativeBGR);

    const auto preferred = [&]() {
        const auto vk_colorSpace = to_vk(requestedColorSpace);

        vk::Format format = isNativeBGR ? vk::Format::eB8G8R8A8Srgb : vk::Format::eR8G8B8A8Srgb;
        switch (requestedColorSpace)
        {
        case ColorSpace::SrgbNonlinear:
            format = isNativeBGR ? vk::Format::eB8G8R8A8Unorm : vk::Format::eR8G8B8A8Unorm;
            break;
        case ColorSpace::ExtendedSrgbLinear:
            format = vk::Format::eR16G16B16A16Sfloat;
            break;
        case ColorSpace::Hdr10:
        case ColorSpace::Bt709Linear:
            break;
        }

        return vk::SurfaceFormatKHR(format, vk_colorSpace);
    }();

    Log::Core::Trace("Available formats:");
    for (const auto& format : physicalDevice.surface.formats)
        Log::Core::Trace(
            "  {} / {}", vk::to_string(format.format), vk::to_string(format.colorSpace));

    Log::Core::Trace(
        "Preferred format: {} / {}",
        vk::to_string(preferred.format),
        vk::to_string(preferred.colorSpace));

    for (const auto& fmt : physicalDevice.surface.formats)
        if (fmt.format == preferred.format && fmt.colorSpace == preferred.colorSpace)
            return fmt;

    Log::Core::Warn("Could not find a native swap chain format with the preferred color space.");

    for (const auto& fmt : physicalDevice.surface.formats)
        if (fmt.format == preferred.format)
            return fmt;

    Log::Core::Warn(
        "Could not find the preferred native swap chain format. Defaulting to first supported "
        "format.");

    return physicalDevice.surface.formats[0];
}

} // namespace VoxelDynamics::Vulkan
