cmake_minimum_required(VERSION 3.31)
project(Sandbox)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(BUILD_SHARED_LIBS OFF)

# ##############################################################################

add_executable(Sandbox src/Sandbox.cpp)

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_BUILD_TYPE_UPPER MATCHES "RELEASE")
  message("Logging is disabled for ${PROJECT_NAME}")
else()
  option(ENABLE_LOGGING "Enable application logging" ON)
  target_compile_definitions(Sandbox PRIVATE ENABLE_LOGGING)
endif()

target_link_libraries(Sandbox PRIVATE VoxelDynamics)

add_custom_target(
  copy_assets
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/" "$<TARGET_FILE_DIR:Sandbox>/assets"
  COMMENT "Copying assets if different...")

add_dependencies(Sandbox copy_assets)

install(TARGETS Sandbox DESTINATION bin)
