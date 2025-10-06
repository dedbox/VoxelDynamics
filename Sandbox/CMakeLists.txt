cmake_minimum_required(VERSION 3.31)
project(Sandbox)

# Sandbox ######################################################################

add_executable(Sandbox src/Sandbox.cpp)

target_link_libraries(Sandbox PRIVATE VoxelDynamics)

# Compile-time Options #########################################################

option(ENABLE_LOGGING "Enable application logging" OFF)
option(ENABLE_DEBUGGING "Enable application debugging instrumentation" OFF)

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_BUILD_TYPE_UPPER MATCHES "RELEASE")
  message("Logging is disabled for ${PROJECT_NAME}")
  message("Debugging is disabled for ${PROJECT_NAME}")
else()
  target_compile_definitions(Sandbox PRIVATE ENABLE_LOGGING ENABLE_DEBUGGING)
endif()
