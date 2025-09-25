cmake_minimum_required(VERSION 3.31)
project(Sandbox)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(BUILD_SHARED_LIBS OFF)

add_executable(Sandbox src/Sandbox.cpp)

target_link_libraries(Sandbox PRIVATE VoxelDynamics glfw Threads::Threads
                                      Taskflow::Taskflow)

install(TARGETS Sandbox DESTINATION bin)
