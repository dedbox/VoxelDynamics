#pragma once

#include "vulkan/vulkan_raii.hpp"

namespace VoxelDynamics::Vulkan
{

/** A block of data allocated in physical memory.
 *
 * A Buffer object contains a Vulkan buffer handle and a device memory handle. The buffer handle
 * defines the size and intended usage of the buffer (e.g., vertex or index data). The device memory
 * handle represents an actual block of memory allocated from a specific memory heap on a physical
 * device such as in GPU RAM or a host-visible memory region.
 */
class Buffer
{
public:
    vk::raii::Buffer buffer;
    vk::raii::DeviceMemory memory;
};

} // namespace VoxelDynamics::Vulkan
