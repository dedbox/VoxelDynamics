#include "VoxelDynamics/Renderer/Vulkan/Device.hpp"

namespace VoxelDynamics::Vulkan
{

Device::Device(
    vk::raii::Device device, vk::raii::Queue graphiceQueue_, vk::raii::Queue computeQueue_)
    : vk_(std::move(device))
    , graphicsQueue(std::move(graphiceQueue_))
    , computeQueue(std::move(computeQueue_))
{
}

Device Device::Create(
    const PhysicalDevice& physicalDevice, const std::vector<const char*>& extraExtensions)
{
    const float queuePriority = 1.0F;

    const std::array<vk::DeviceQueueCreateInfo, 2> queueCreateInfo{
        vk::DeviceQueueCreateInfo({}, physicalDevice.graphicsQueueFamilyIndex, 1, &queuePriority),
        vk::DeviceQueueCreateInfo({}, physicalDevice.computeQueueFamilyIndex, 1, &queuePriority),
    };

    const uint32_t numQueues =
        physicalDevice.graphicsQueueFamilyIndex == physicalDevice.computeQueueFamilyIndex ? 1 : 2;
    const auto extensions = PhysicalDevice::Extensions(extraExtensions);

    vk::DeviceCreateInfo createInfo(
        {},
        numQueues,
        queueCreateInfo.data(),
        0,
        nullptr,
        extensions.size(),
        extensions.data(),
        &physicalDevice.features.get<vk::PhysicalDeviceFeatures2>().features,
        physicalDevice.features.get<vk::PhysicalDeviceVulkan13Features>());

    vk::raii::Device device(*physicalDevice, createInfo);

    Log::Core::Info("Logical device created");

    vk::raii::Queue graphicsQueue = device.getQueue(physicalDevice.graphicsQueueFamilyIndex, 0);
    vk::raii::Queue computeQueue  = device.getQueue(physicalDevice.computeQueueFamilyIndex, 0);

    Log::Core::Info("Device queues created");

    return Device(std::move(device), std::move(graphicsQueue), std::move(computeQueue));
}

void Device::setDebugName(vk::ObjectType type, uint64_t handle, const std::string& name) const
{
    vk_.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT(type, handle, name.c_str()));
}

} // namespace VoxelDynamics::Vulkan
