cmake_minimum_required(VERSION 3.31)
project(stb)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_SHALLOW YES)

FetchContent_MakeAvailable(stb)

add_library(stb_image_lib STATIC stb_image.cpp)

target_include_directories(stb_image_lib PUBLIC ${stb_SOURCE_DIR})
