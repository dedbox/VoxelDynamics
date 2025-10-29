cmake_minimum_required(VERSION 3.31)
project(VoxelDynamics)

add_library(
  VoxelDynamics STATIC
  src/VoxelDynamics/Core/Application.cpp
  src/VoxelDynamics/Core/Log.cpp
  src/VoxelDynamics/Core/Time.cpp
  src/VoxelDynamics/Core/Util.cpp
  src/VoxelDynamics/Core/Window.cpp
  src/VoxelDynamics/Vulkan/CommandBufferManager.cpp
  src/VoxelDynamics/Vulkan/Context.cpp
  src/VoxelDynamics/Vulkan/Instance.cpp
  src/VoxelDynamics/Vulkan/PhysicalDevice.cpp
  src/VoxelDynamics/Vulkan/PipelineManager.cpp
  src/VoxelDynamics/Vulkan/Renderer.cpp
  vendor/spirv-reflect/spirv_reflect.c)

target_link_libraries(VoxelDynamics PUBLIC Vulkan::Vulkan SDL3::SDL3 EnTT::EnTT
                                           spdlog::spdlog)

target_include_directories(
  VoxelDynamics
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
         ${CMAKE_CURRENT_SOURCE_DIR}/vendor/spirv-reflect ${glm_SOURCE_DIR})

# SPIRV-Reflect Config #########################################################

set_source_files_properties(vendor/spirv-reflect/spirv_reflect.c
                            PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)

target_compile_definitions(VoxelDynamics
                           PRIVATE SPIRV_REFLECT_USE_SYSTEM_SPIRV_H)

# Compile-time Options #########################################################

option(ENABLE_LOGGING "Enable application logging" OFF)
option(ENABLE_DEBUGGING "Enable application debugging instrumentation" OFF)

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_BUILD_TYPE_UPPER MATCHES "RELEASE")
  message("Logging is disabled for ${PROJECT_NAME}")
  message("Debugging is disabled for ${PROJECT_NAME}")
else()
  target_compile_definitions(VoxelDynamics PRIVATE ENABLE_LOGGING
                                                   ENABLE_DEBUGGING)
endif()

# Precompiled Headers ##########################################################

target_precompile_headers(
  VoxelDynamics
  PUBLIC
  <algorithm>
  <csignal>
  <cstddef>
  <cstdint>
  <filesystem>
  <format>
  <fstream>
  <functional>
  <ios>
  <iterator>
  <limits>
  <list>
  <map>
  <memory>
  <optional>
  <print>
  <ranges>
  <set>
  <span>
  <source_location>
  <stdexcept>
  <string>
  <string_view>
  <tuple>
  <type_traits>
  <typeindex>
  <utility>
  <unordered_map>
  <unordered_set>
  <variant>
  <vector>
  src/VoxelDynamics/Config.hpp
  src/VoxelDynamics/Core/Exception.hpp
  src/VoxelDynamics/Core/Log.hpp)
