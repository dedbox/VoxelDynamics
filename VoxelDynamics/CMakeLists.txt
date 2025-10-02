cmake_minimum_required(VERSION 3.31)
project(VoxelDynamics)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(BUILD_SHARED_LIBS OFF)

add_subdirectory(stb)

# ##############################################################################

add_library(
  VoxelDynamics STATIC
  src/VoxelDynamics/Application.cpp
  src/VoxelDynamics/Core/Log.cpp
  src/VoxelDynamics/Core/Window.cpp
  src/VoxelDynamics/Renderer/Vulkan/Context.cpp
  src/VoxelDynamics/Renderer/Vulkan/Device.cpp
  src/VoxelDynamics/Renderer/Vulkan/Instance.cpp
  src/VoxelDynamics/Renderer/Vulkan/PhysicalDevice.cpp
  src/VoxelDynamics/Shader/ShaderCompiler.cpp)

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

target_link_libraries(
  VoxelDynamics
  PRIVATE glfw
          Threads::Threads
          Taskflow::Taskflow
          Vulkan::Vulkan
          Vulkan::glslang
          glslang-default-resource-limits
          spdlog::spdlog)

target_link_libraries(VoxelDynamics PUBLIC stb_image_lib ktx)

target_include_directories(VoxelDynamics INTERFACE ${glfw_SOURCE_DIR}/include
                                                   ${Vulkan_INCLUDE_DIRS})

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
  src/VoxelDynamics/Core/Bitmask.hpp
  src/VoxelDynamics/Core/Log.hpp)

target_include_directories(VoxelDynamics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS VoxelDynamics DESTINATION lib)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.cpp" EXCLUDE)
