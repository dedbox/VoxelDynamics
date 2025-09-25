cmake_minimum_required(VERSION 3.31)
project(Sandbox)

set(CMAKE_CXX_STANDARD 23)

set(BUILD_SHARED_LIBS OFF)

add_executable(Sandbox src/Sandbox.cpp)

target_link_libraries(Sandbox PRIVATE VoxelDynamics)

install(TARGETS Sandbox DESTINATION bin)
