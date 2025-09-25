cmake_minimum_required(VERSION 3.31)
project(Sandbox)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(BUILD_SHARED_LIBS OFF)

add_executable(Sandbox src/Sandbox.cpp)

if(ENABLE_LOGGING)
  target_compile_definitions(Sandbox PRIVATE ENABLE_LOGGING)
endif()

target_link_libraries(
  Sandbox
  PRIVATE VoxelDynamics
          glfw
          Threads::Threads
          Taskflow::Taskflow
          Vulkan::Vulkan
          Vulkan::glslang
          Vulkan::SPIRV-Tools)

install(TARGETS Sandbox DESTINATION bin)
