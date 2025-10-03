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
    , _requestedColorSpace(createInfo.requestedColorSpace)
    , _swapChain(
          SwapChain::Create(
              _physicalDevice,
              _device,
              createInfo.width,
              createInfo.height,
              createInfo.requestedColorSpace))
    , _timelineSemaphore(createTimelineSemaphore())
{
}

void Context::resizeSwapChain(uint32_t width, uint32_t height)
{
    _device.handle.waitIdle();
    _swapChain.destroy();
    _swapChain = SwapChain::Create(_physicalDevice, _device, width, height, _requestedColorSpace);
}

vk::raii::Semaphore Context::createTimelineSemaphore() const
{
    const vk::SemaphoreTypeCreateInfo typeCreateInfo(
        vk::SemaphoreType::eTimeline, _swapChain.images.size() - 1);

    const vk::SemaphoreCreateInfo createInfo;

    const vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo> createInfoChain(
        createInfo, typeCreateInfo);

    vk::raii::Semaphore semaphore =
        _device.handle.createSemaphore(createInfoChain.get<vk::SemaphoreCreateInfo>());
    _device.setDebugName(
        vk::ObjectType::eSemaphore,
        reinterpret_cast<uint64_t>(&**semaphore), // NOLINT
        "Timeline Semaphore");

    return semaphore;
}

} // namespace VoxelDynamics::Vulkan
