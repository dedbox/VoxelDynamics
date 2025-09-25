cmake_minimum_required(VERSION 3.31)
project(VoxelDynamics)

set(CMAKE_CXX_STANDARD 23)

set(BUILD_SHARED_LIBS OFF)

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

# Taskflow #####################################################################

FetchContent_Declare(
  taskflow
  GIT_REPOSITORY https://github.com/taskflow/taskflow.git
  # GIT_TAG v3.10.0
  GIT_SHALLOW ON)

set(TF_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(TF_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(taskflow)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

# Vulkan #######################################################################

find_package(Vulkan REQUIRED COMPONENTS glslang SPIRV-Tools)

if(NOT Vulkan_FOUND)
  message(FATAL_ERROR "Vulkan not found.")
endif()

if(NOT (TARGET Vulkan::glslang))
  message(FATAL_ERROR "glslang not found.")
endif()

if(NOT (TARGET Vulkan::SPIRV-Tools))
  message(FATAL_ERROR "SPIRV-Tools not found.")
endif()

# spdlog #######################################################################

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.3
  GIT_SHALLOW YES)

FetchContent_MakeAvailable(spdlog)

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_BUILD_TYPE_UPPER MATCHES "RELEASE")
  set(ENABLE_LOGGING_DEFAULT ON)
else()
  set(ENABLE_LOGGING_DEFAULT OFF)
endif()

option(ENABLE_LOGGING "Enable application logging" ${ENABLE_LOGGING_DEFAULT})

# ##############################################################################

add_subdirectory(VoxelDynamics)
add_subdirectory(Sandbox)
