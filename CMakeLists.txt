cmake_minimum_required(VERSION 3.31)
project(VoxelDynamics)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(BUILD_SHARED_LIBS OFF)

# External Dependencies  #######################################################

include(FetchContent)

# Vulkan -----------------------------------------------------------------------

find_package(Vulkan REQUIRED)

if(NOT Vulkan_FOUND)
  message(FATAL_ERROR "Vulkan not found.")
endif()

# LVK --------------------------------------------------------------------------

FetchContent_Declare(
  lightweightvk
  GIT_REPOSITORY https://github.com/corporateshark/lightweightvk.git
  GIT_SHALLOW ON)

set(GLFW_BUILD_DOCS
    OFF
    CACHE BOOL "" FORCE)

set(GLFW_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)

set(GLFW_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(lightweightvk)

# spdlog -----------------------------------------------------------------------

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.3
  GIT_SHALLOW YES)

FetchContent_MakeAvailable(spdlog)

# ##############################################################################

add_subdirectory(VoxelDynamics)
add_subdirectory(Sandbox)
