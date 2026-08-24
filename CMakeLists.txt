cmake_minimum_required(VERSION 3.15)
project(glvk VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Dependencies: OpenGL, EGL
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EGL REQUIRED egl)

# SPIRV-Cross dependency via FetchContent
include(FetchContent)
FetchContent_Declare(
    spirv_cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
    GIT_TAG        main
)
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_GLSL ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spirv_cross)

# GLVK Library Source Files
set(GLVK_SOURCES
    src/gl_backend.cpp
    src/glvk_instance.cpp
    src/glvk_device.cpp
    src/glvk_memory.cpp
    src/glvk_buffer.cpp
    src/glvk_shader.cpp
    src/glvk_pipeline.cpp
    src/glvk_descriptor.cpp
    src/glvk_command.cpp
    src/glvk_sync.cpp
    src/glvk_entrypoints.cpp
)

# Build both static library and shared library (drop-in libvulkan.so)
add_library(glvk_static STATIC ${GLVK_SOURCES})
target_include_directories(glvk_static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${EGL_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)
target_link_libraries(glvk_static PUBLIC
    spirv-cross-glsl
    spirv-cross-core
    ${EGL_LIBRARIES}
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

add_library(vulkan SHARED ${GLVK_SOURCES})
set_target_properties(vulkan PROPERTIES
    OUTPUT_NAME "vulkan"
    SOVERSION "1"
    VERSION "1.3.0"
)
target_include_directories(vulkan PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${EGL_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)
target_link_libraries(vulkan PUBLIC
    spirv-cross-glsl
    spirv-cross-core
    ${EGL_LIBRARIES}
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

# Build Tests
enable_testing()
add_subdirectory(tests)
