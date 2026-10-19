cmake_minimum_required(VERSION 3.16)
project(GpuFluid LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(GPUFLUID_SYSTEM_GLFW "Use an installed GLFW instead of fetching it" OFF)

find_package(OpenGL REQUIRED)

if(GPUFLUID_SYSTEM_GLFW)
  find_package(glfw3 3.3 REQUIRED)
else()
  include(FetchContent)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(glfw)
endif()

# glad: OpenGL 3.3 core loader (generated, vendored in external/glad)
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

add_executable(GpuFluid
  src/Main.cpp
  src/FluidSimulator.cpp
  src/OpenGL.cpp)
target_include_directories(GpuFluid PRIVATE src)
target_include_directories(GpuFluid SYSTEM PRIVATE external/stb)
target_link_libraries(GpuFluid PRIVATE glad glfw OpenGL::GL)
target_compile_definitions(GpuFluid PRIVATE
  GPUFLUID_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders/")
if(MSVC)
  target_compile_options(GpuFluid PRIVATE /W3)
  target_compile_definitions(GpuFluid PRIVATE _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES NOMINMAX)
else()
  target_compile_options(GpuFluid PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
if(APPLE)
  target_compile_definitions(GpuFluid PRIVATE GL_SILENCE_DEPRECATION)
endif()

# Copy shaders next to the binary so it also runs from the build folder
add_custom_command(TARGET GpuFluid POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/Shaders $<TARGET_FILE_DIR:GpuFluid>/Shaders)
