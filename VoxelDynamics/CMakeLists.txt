cmake_minimum_required(VERSION 3.31)
project(VoxelDynamics)

add_library(VoxelDynamics STATIC src/VoxelDynamics/Core/Application.cpp
                                 src/VoxelDynamics/Core/Log.cpp)

target_link_libraries(VoxelDynamics PRIVATE LVKLibrary spdlog::spdlog)

target_include_directories(VoxelDynamics INTERFACE ${Vulkan_INCLUDE_DIRS}
                                                   ${spdlog_SOURCE_DIR}/include)

target_include_directories(
  VoxelDynamics
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
         ${lightweightvk_SOURCE_DIR}/third-party/deps/src/glfw/include)

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
  src/VoxelDynamics/Core/Log.hpp)
