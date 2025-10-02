#include "VoxelDynamics/Renderer/Vulkan/Context.hpp"

namespace VoxelDynamics::Vulkan
{

Context::Context(const Window& window, const CreateInfo& createInfo)
    : _instance(Instance(_context, createInfo.appName, createInfo.appVersion))
    , _surface(vk::raii::SurfaceKHR(*_instance, window.createSurface(**_instance)))
    , _physicalDevice(
          PhysicalDevice::Create(
              *_instance, _surface, createInfo.preferredDeviceType, createInfo.deviceExtensions))
    , _device(Device::Create(_physicalDevice, createInfo.deviceExtensions))
{
}

} // namespace VoxelDynamics::Vulkan
