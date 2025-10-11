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

# SDL --------------------------------------------------------------------------

FetchContent_Declare(
  SDL
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.24
  GIT_SHALLOW YES)

set(SDL_SHARED OFF)
set(SDL_STATIC ON)

FetchContent_MakeAvailable(SDL)

# entt -------------------------------------------------------------------------

FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.15.0
  GIT_SHALLOW YES)

FetchContent_MakeAvailable(entt)

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
