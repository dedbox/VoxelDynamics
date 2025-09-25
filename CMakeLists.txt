cmake_minimum_required(VERSION 3.31)
project(VoxelDynamics)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

# GLFW #########################################################################

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW YES)

set(GLFW_BUILD_DOCS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# ##############################################################################

add_subdirectory(VoxelDynamics)
add_subdirectory(Sandbox)
