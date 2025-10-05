cmake_minimum_required(VERSION 3.31)
project(VoxelDynamics)

add_library(VoxelDynamics STATIC src/VoxelDynamics/hello.cpp)

target_include_directories(VoxelDynamics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

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
  <vector>)
