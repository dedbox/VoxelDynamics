cmake_minimum_required(VERSION 3.31)
project(VoxelDynamics)

set(CMAKE_CXX_STANDARD 23)

set(BUILD_SHARED_LIBS OFF)

# ##############################################################################

add_library(VoxelDynamics STATIC src/VoxelDynamics/Core/Log.cpp)

if(ENABLE_LOGGING)
  target_compile_definitions(VoxelDynamics PRIVATE ENABLE_LOGGING)
endif()

target_link_libraries(VoxelDynamics PRIVATE glfw spdlog::spdlog)

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
  <iterator>
  <list>
  <map>
  <memory>
  <optional>
  <print>
  <ranges>
  <span>
  <stdexcept>
  <string>
  <tuple>
  <type_traits>
  <typeindex>
  <utility>
  <unordered_map>
  <unordered_set>
  <variant>
  <vector>
  src/VoxelDynamics/Core/Log.hpp)

target_include_directories(VoxelDynamics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS VoxelDynamics DESTINATION lib)

# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/VoxelDynamics.hpp DESTINATION
# include)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.cpp" EXCLUDE)
