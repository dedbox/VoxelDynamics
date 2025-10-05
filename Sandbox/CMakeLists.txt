cmake_minimum_required(VERSION 3.31)
project(Sandbox)

add_executable(Sandbox src/Sandbox.cpp)

target_link_libraries(Sandbox PRIVATE VoxelDynamics)
