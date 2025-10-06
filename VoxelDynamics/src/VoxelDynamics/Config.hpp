#pragma once

#include "vulkan/vulkan.hpp"

namespace VoxelDynamics
{

#ifndef ENABLE_LOGGING
inline constexpr bool is_logging_enabled = false;
#else
inline constexpr bool is_logging_enabled = true;
#endif

#ifndef ENABLE_DEBUGGING
inline constexpr bool is_debugging_enabled = false;
#else
inline constexpr bool is_debugging_enabled = true;
#endif

constexpr uint32_t Version(uint32_t major, uint32_t minor, uint32_t patch)
{
    return vk::makeVersion(major, minor, patch);
}

constexpr std::string EngineName = "VoxelDynamics";
constexpr uint32_t EngineVersion = Version(1, 0, 0);

} // namespace VoxelDynamics
