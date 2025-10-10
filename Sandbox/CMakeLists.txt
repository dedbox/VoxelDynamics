cmake_minimum_required(VERSION 3.31)
project(Sandbox)

# Shaders ######################################################################

find_program(SLANGC_EXECUTABLE slangc)
if(NOT SLANGC_EXECUTABLE)
  message(FATAL_ERROR "could not find slangc executable.")
endif()

set(SLANG_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/shaders/slang.slang)
set(SPIRV_OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/shaders/slang.slang.spv)

add_custom_command(
  OUTPUT ${SPIRV_OUTPUT_FILE}
  COMMAND ${SLANGC_EXECUTABLE} ARGS ${SLANG_SOURCE_FILE} -target spirv -o
          ${SPIRV_OUTPUT_FILE}
  DEPENDS ${SLANG_SOURCE_FILE}
  COMMENT "Compiling ${SLANG_SOURCE_FILE} to ${SPIRV_OUTPUT_FILE}")

# Sandbox ######################################################################

add_executable(Sandbox src/Sandbox.cpp)

target_link_libraries(Sandbox PRIVATE VoxelDynamics)

target_sources(Sandbox PRIVATE ${SPIRV_OUTPUT_FILE})

# add_dependencies(Sandbox CompileSlang)

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
