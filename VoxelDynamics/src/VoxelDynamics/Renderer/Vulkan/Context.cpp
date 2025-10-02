#include "VoxelDynamics/Renderer/Vulkan/Context.hpp"

namespace VoxelDynamics::Vulkan
{

Context::Context(const Window& window, const CreateInfo& createInfo)
    : _instance(_context, createInfo.appName, createInfo.appVersion)
    , _physicalDevice(
          PhysicalDevice::Create(
              *_instance,
              vk::raii::SurfaceKHR(*_instance, window.createSurface(**_instance)),
              createInfo.preferredDeviceType,
              createInfo.deviceExtensions))
    , _device(Device::Create(_physicalDevice, createInfo.deviceExtensions))
{
}

} // namespace VoxelDynamics::Vulkan
