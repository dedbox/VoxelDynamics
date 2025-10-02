#include "VoxelDynamics/Renderer/Vulkan/Context.hpp"

namespace VoxelDynamics::Vulkan
{

Context::Context(const Window& window, const CreateInfo& createInfo)
    : _instance(Instance(_context, createInfo.appName, createInfo.appVersion))
    , _surface(vk::raii::SurfaceKHR(*_instance, window.createSurface(**_instance)))
    , _physicalDevice(
          PhysicalDevice::Create(
              *_instance, _surface, createInfo.preferredDeviceType, createInfo.deviceExtensions))
    , _device(CreateDevice(createInfo.deviceExtensions))
{
}

// CreateDevice
// ////////////////////////////////////////////////////////////////////////////////////

Context::Device Context::CreateDevice(const std::vector<const char*>& deviceExtensions)
{
    const float queuePriority = 1.0F;

    const std::array<vk::DeviceQueueCreateInfo, 2> queueCreateInfo{
        vk::DeviceQueueCreateInfo({}, _physicalDevice.graphicsQueueFamilyIndex, 1, &queuePriority),
        vk::DeviceQueueCreateInfo({}, _physicalDevice.computeQueueFamilyIndex, 1, &queuePriority),
    };

    const uint32_t numQueues =
        _physicalDevice.graphicsQueueFamilyIndex == _physicalDevice.computeQueueFamilyIndex ? 1 : 2;
    const auto extensions = PhysicalDevice::Extensions(deviceExtensions);

    vk::DeviceCreateInfo createInfo(
        {},
        numQueues,
        queueCreateInfo.data(),
        0,
        nullptr,
        extensions.size(),
        extensions.data(),
        &_physicalDevice.features.get<vk::PhysicalDeviceFeatures2>().features,
        _physicalDevice.features.get<vk::PhysicalDeviceVulkan13Features>());

    vk::raii::Device device(*_physicalDevice, createInfo);

    Log::Core::Info("Logical device created");

    vk::raii::Queue graphicsQueue = device.getQueue(_physicalDevice.graphicsQueueFamilyIndex, 0);
    vk::raii::Queue computeQueue  = device.getQueue(_physicalDevice.computeQueueFamilyIndex, 0);

    Log::Core::Info("Device queues created");

    return Device(std::move(device), std::move(graphicsQueue), std::move(computeQueue));
}

} // namespace VoxelDynamics::Vulkan
