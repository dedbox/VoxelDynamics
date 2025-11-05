#include "SDL3/SDL_vulkan.h"

#include "VoxelDynamics/Vulkan/Context.hpp"

namespace VoxelDynamics::Vulkan
{

Context::Context(
    BuildInfo buildInfo_, const std::string& appName, const uint64_t appVersion, Window& window)
    : buildInfo(buildInfo_)
    , _instance(Instance(_context, appName, appVersion))
    , _surface(createSurface(window))
    , _physicalDevice(pickPhysicalDevice())
    , _device(Device(_physicalDevice))
{
}

vk::raii::SurfaceKHR Context::createSurface(Window& window) const
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if (!SDL_Vulkan_CreateSurface(*window, **_instance, nullptr, &surface))
        throw SDLException("Could not create Vulkan surface");

    return vk::raii::SurfaceKHR(*_instance, surface);
}

PhysicalDevice Context::pickPhysicalDevice() const
{
    Log::Core::Trace("Picking a physical device...");

    // report required extensions
    const auto extensions = PhysicalDevice::RequiredDeviceExtensions();
    for (const auto& ext : extensions)
        Log::Core::Trace("Required device extension: {}", ext);

    // find all physical devices
    const auto allPhysicalDevices = _instance->enumeratePhysicalDevices();
    Log::Core::Info("Found {} physical devices:", allPhysicalDevices.size());

    for (auto&& [i, physicalDevice] : std::ranges::views::enumerate(allPhysicalDevices))
    {
        const auto props = physicalDevice.getProperties();
        Log::Core::Info(
            "  [{}] {} ({}), API version {}.{}.{}",
            i + 1,
            std::string(props.deviceName),
            vk::to_string(props.deviceType),
            vk::apiVersionMajor(props.apiVersion),
            vk::apiVersionMinor(props.apiVersion),
            vk::apiVersionPatch(props.apiVersion));
    }

    struct SuitablePhysicalDevice
    {
        uint32_t i;
        PhysicalDevice physicalDevice;
    };

    // partition suitable devices by type
    std::array<std::vector<SuitablePhysicalDevice>, 3> physicalDevices;
    size_t binSize = allPhysicalDevices.size() / 3;
    for (auto& bin : physicalDevices)
        bin.reserve(binSize);

    std::map<vk::PhysicalDeviceType, int> binIndex{{
        {vk::PhysicalDeviceType::eDiscreteGpu, 0},
        {vk::PhysicalDeviceType::eIntegratedGpu, 1},
        {vk::PhysicalDeviceType::eCpu, 2},
    }};

    std::array<int, 3> binOrder{};
    switch (buildInfo.preferredDeviceType)
    {
    case vk::PhysicalDeviceType::eDiscreteGpu:
        binOrder = {0, 1, 2};
        break;
    case vk::PhysicalDeviceType::eIntegratedGpu:
        binOrder = {1, 0, 2};
        break;
    case vk::PhysicalDeviceType::eCpu:
        binOrder = {2, 1, 0};
        break;
    case vk::PhysicalDeviceType::eOther:
    case vk::PhysicalDeviceType::eVirtualGpu:
        throw std::runtime_error("Refusing to pick a virtual device");
    }

    for (auto&& [i, raw_physicalDevice] : std::ranges::views::enumerate(allPhysicalDevices))
    {
        PhysicalDevice physicalDevice(raw_physicalDevice, _surface);

        // skip headless devices
        if (physicalDevice.properties.deviceType == vk::PhysicalDeviceType::eOther ||
            physicalDevice.properties.deviceType == vk::PhysicalDeviceType::eVirtualGpu)
        {
            Log::Core::Trace("Skipping device {}: headless", i + 1);
            continue;
        }

        // check for Vulkan 1.3 support
        if (physicalDevice.properties.apiVersion < vk::ApiVersion13)
        {
            Log::Core::Trace("Skipping device {}: does not support Vulkan 1.3", i + 1);
            continue;
        }

        // check required device extensions
        if (!physicalDevice.checkDeviceExtensions())
        {
            Log::Core::Trace("Skipping device {}: missing required extensions", i + 1);
            continue;
        }

        // check queue families
        if (!physicalDevice.index)
        {
            Log::Core::Trace(
                "Skipping device {}: graphics / present operations not supported", i + 1);
            continue;
        }

        // check for surface format compatibility
        if (physicalDevice.surfaceFormats.empty())
        {
            Log::Core::Trace("Skipping device {}: no compatible surface formats", i + 1);
            continue;
        }

        // check require device features
        if (!physicalDevice.features)
        {
            Log::Core::Trace("Skipping device {}: missing required features", i + 1);
            continue;
        }

        // bin suitable candidate
        physicalDevices[binIndex[physicalDevice.properties.deviceType]].emplace_back(
            i, std::move(physicalDevice));
    }

    // pick the first suitable candidate
    for (const size_t bin : binOrder)
    {
        if (physicalDevices[bin].empty())
            continue;

        auto& pd = physicalDevices[bin].front();

        Log::Core::Info(
            "Picked physical device {}: {} ({}), API version {}.{}.{}",
            pd.i + 1,
            std::string(pd.physicalDevice.properties.deviceName),
            vk::to_string(pd.physicalDevice.properties.deviceType),
            vk::apiVersionMajor(pd.physicalDevice.properties.apiVersion),
            vk::apiVersionMinor(pd.physicalDevice.properties.apiVersion),
            vk::apiVersionPatch(pd.physicalDevice.properties.apiVersion));
        Log::Core::Info("  formats: {}", SurfaceFormatNames(pd.physicalDevice.surfaceFormats));
        Log::Core::Info("  present modes: {}", PresentModeNames(pd.physicalDevice.presentModes));
        Log::Core::Info("  transfer queue family index: {}", pd.physicalDevice.index->transfer);
        Log::Core::Info("  graphics queue family index: {}", pd.physicalDevice.index->graphics);
        Log::Core::Info("  present queue family index: {}", pd.physicalDevice.index->present);

        for (const auto& ext : pd.physicalDevice.availableExtensions)
            Log::Core::Trace("Available device extension: {}", ext);

        return std::move(pd.physicalDevice);
    }

    throw std::runtime_error("No suitable physical device found");
}

Buffer Context::createBuffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    const std::string& bufferName,
    const std::string& memoryName) const
{
    // create buffer handle
    vk::BufferCreateInfo createInfo({}, size, usage, vk::SharingMode::eExclusive);
    vk::raii::Buffer buffer(*_device, createInfo);

    setDebugName(vk::ObjectType::eBuffer, &**buffer, bufferName);

    // allocate buffer memory
    const auto memReqs = buffer.getMemoryRequirements();
    const auto memType = findMemoryType(memReqs.memoryTypeBits, properties);

    vk::MemoryAllocateInfo allocInfo(memReqs.size, memType);
    vk::raii::DeviceMemory memory(*_device, allocInfo);

    setDebugName(vk::ObjectType::eDeviceMemory, &**memory, memoryName);

    // associate the memory with the handle
    buffer.bindMemory(*memory, 0);

    return Buffer(std::move(buffer), std::move(memory));
}

uint32_t Context::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
    // query available memory types
    vk::PhysicalDeviceMemoryProperties memProps = _physicalDevice->getMemoryProperties();

    // find a suitable type
    for (const auto& [i, memType] : std::ranges::views::enumerate(memProps.memoryTypes))
        if ((typeFilter & (1U << static_cast<uint32_t>(i))) &&
            (memType.propertyFlags & properties) == properties)
            return i;

    throw std::runtime_error("Could not find a suitable memory type");
}

std::string Context::SurfaceFormatName(const vk::SurfaceFormatKHR& format)
{
    return vk::to_string(format.format) + " / " + vk::to_string(format.colorSpace);
}

std::string Context::SurfaceFormatNames(const std::vector<vk::SurfaceFormatKHR>& formats)
{
    return formats | std::ranges::views::transform(SurfaceFormatName) |
           std::ranges::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>();
}

std::string Context::PresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes)
{
    const auto modeName = [](const vk::PresentModeKHR& mode) { return vk::to_string(mode); };

    return presentModes | std::ranges::views::transform(modeName) |
           std::ranges::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>();
}

void Context::setDebugName(vk::ObjectType type, void* handle, const std::string& name) const
{
    _device->setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT(type, reinterpret_cast<uint64_t>(handle), name.c_str()));
}

} // namespace VoxelDynamics::Vulkan
