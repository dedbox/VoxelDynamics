#include "VoxelDynamics/Vulkan/PhysicalDevice.hpp"

namespace VoxelDynamics::Vulkan
{

PhysicalDevice::PhysicalDevice(
    vk::raii::PhysicalDevice physicalDevice_, const vk::raii::SurfaceKHR& surface)
    : physicalDevice(std::move(physicalDevice_))
    , properties(physicalDevice.getProperties())
    , availableExtensions(getAvailableExtensions())
    , index(findQueueFamilies(surface))
    , surfaceFormats(physicalDevice.getSurfaceFormatsKHR(surface))
    , presentModes(physicalDevice.getSurfacePresentModesKHR(surface))
    , features(createFeaturesChain())
{
}

std::vector<uint32_t> PhysicalDevice::uniqueQueueFamilyIndices() const
{
    std::set<uint32_t> indices;
    if (index)
    {
        indices.emplace(index->transfer);
        indices.emplace(index->graphics);
        indices.emplace(index->present);
    }
    return indices | std::ranges::to<std::vector>();
}

std::vector<std::string> PhysicalDevice::getAvailableExtensions() const
{
    const auto extensionName = [](const vk::ExtensionProperties& p) {
        return std::string(p.extensionName);
    };

    return physicalDevice.enumerateDeviceExtensionProperties() |
           std::ranges::views::transform(extensionName) |
           std::ranges::to<std::vector<std::string>>();
}

bool PhysicalDevice::checkDeviceExtensions() const
{
    const auto& requiredExtensions = RequiredDeviceExtensions();

    std::set<std::string> missing(requiredExtensions.begin(), requiredExtensions.end());

    for (const auto& layer : availableExtensions)
        missing.erase(layer);

    if (!missing.empty())
    {
        for (const auto& ext : missing)
            Log::Core::Warn("Missing device extension: {}", ext);
        return false;
    }

    return true;
}

std::optional<PhysicalDevice::QueueFamilyIndices> PhysicalDevice::findQueueFamilies(
    const vk::raii::SurfaceKHR& surface) const
{
    std::optional<uint32_t> transfer, graphics, present;

    for (const auto& [i, queueFamily] :
         std::ranges::views::enumerate(physicalDevice.getQueueFamilyProperties()))
    {
        // check for graphics support
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            graphics = i;

        // check for presentation support
        if (physicalDevice.getSurfaceSupportKHR(i, *surface))
            present = i;

        // check for dedicated transfer support
        if ((queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) &&
            !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
            !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute))
            transfer = i;

        if (transfer && graphics && present)
            break;
    }

    // if no dedicated transfer support found, use the graphics queue
    if (!transfer)
        transfer = graphics;

    if (!(graphics && present))
        return std::nullopt;

    return QueueFamilyIndices(*transfer, *graphics, *present);
}

std::optional<PhysicalDevice::FeaturesChain> PhysicalDevice::createFeaturesChain()
{
    const auto [h10, have13, have11, haveEDS] = physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    const auto have10 = h10.features;

    vk::PhysicalDeviceFeatures2 want10;
    vk::PhysicalDeviceVulkan13Features want13;
    vk::PhysicalDeviceVulkan11Features want11;
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT wantEDS;

    bool hasAllRequired = true;

    auto require = [&](const vk::Bool32 source, vk::Bool32& dest, const std::string& name) {
        if (source == vk::True)
            dest = vk::True;
        else
        {
            Log::Core::Trace("Missing feature: {}", name);
            hasAllRequired = false;
        }
    };

    require(have11.shaderDrawParameters, want11.shaderDrawParameters, "shaderDrawParameters (1.1)");

    require(have13.synchronization2, want13.synchronization2, "synchronization2 (1.3)");
    require(have13.dynamicRendering, want13.dynamicRendering, "dynamicRendering (1.3)");
    require(
        haveEDS.extendedDynamicState, wantEDS.extendedDynamicState, "extendedDynamicState (EDS)");

    if (hasAllRequired)
        return FeaturesChain(want10, want13, want11, wantEDS);

    return std::nullopt;
}

} // namespace VoxelDynamics::Vulkan
