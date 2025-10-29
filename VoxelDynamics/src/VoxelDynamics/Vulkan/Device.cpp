#include "VoxelDynamics/Vulkan/Device.hpp"

namespace VoxelDynamics::Vulkan
{

Device::Device(const PhysicalDevice& physicalDevice)
    : device(CreateDevice(physicalDevice))
    , graphicsQeeue(createQueue(physicalDevice.index->graphics, "Graphics Queue"))
    , presentQeeue(createQueue(physicalDevice.index->present, "Present Queue"))
    , transferQeeue(createQueue(physicalDevice.index->transfer, "Transfer Queue"))
{
}

vk::raii::Device Device::CreateDevice(const PhysicalDevice& physicalDevice)
{
    const float queuePriority   = 1.0;
    const auto queueCreateInfos = [&]() -> std::vector<vk::DeviceQueueCreateInfo> {
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(3);

        queueCreateInfos.push_back(
            vk::DeviceQueueCreateInfo({}, physicalDevice.index->graphics, 1, &queuePriority));

        if (physicalDevice.index->present != physicalDevice.index->graphics)
        {
            const auto createInfo =
                vk::DeviceQueueCreateInfo({}, physicalDevice.index->present, 1, &queuePriority);
            queueCreateInfos.push_back(createInfo);
        }

        if (physicalDevice.index->transfer != physicalDevice.index->graphics)
        {
            const auto createInfo =
                vk::DeviceQueueCreateInfo({}, physicalDevice.index->transfer, 1, &queuePriority);
            queueCreateInfos.push_back(createInfo);
        }

        return queueCreateInfos;
    }();

    const auto extensions = PhysicalDevice::RequiredDeviceExtensions();
    const auto features   = physicalDevice.features->get<vk::PhysicalDeviceFeatures2>().features;
    const auto features13 = physicalDevice.features->get<vk::PhysicalDeviceVulkan13Features>();

    const vk::DeviceCreateInfo createInfo(
        {},
        queueCreateInfos.size(),
        queueCreateInfos.data(),
        0,
        nullptr,
        extensions.size(),
        extensions.data(),
        &features,
        features13);

    vk::raii::Device device_(physicalDevice.physicalDevice, createInfo);

    device_.setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT(
            vk::ObjectType::eDevice, reinterpret_cast<uint64_t>(&**device_), "Vulkan Device"));

    return device_;
}

vk::raii::Queue Device::createQueue(uint32_t index, const std::string& debugName) const
{
    vk::raii::Queue queue(device, index, 0);

    device.setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT(
            vk::ObjectType::eQueue, reinterpret_cast<uint64_t>(&**queue), debugName.c_str()));

    return std::move(queue);
}

} // namespace VoxelDynamics::Vulkan
