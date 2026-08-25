add_executable(test_vector_add test_vector_add.cpp)
target_link_libraries(test_vector_add PRIVATE glvk_static)
add_test(NAME test_vector_add COMMAND test_vector_add)

add_executable(test_spec_constant test_spec_constant.cpp)
target_link_libraries(test_spec_constant PRIVATE glvk_static)
add_test(NAME test_spec_constant COMMAND test_spec_constant)

set(NCNN_DIR "/home/user/repos/rife-ncnn-vulkan/src/ncnn")
set(NCNN_BUILD_DIR "/home/user/repos/rife-ncnn-vulkan/build/ncnn")
set(NCNN_LIB "${NCNN_BUILD_DIR}/src/libncnn.a")

if(EXISTS "${NCNN_LIB}")
    add_executable(test_ncnn_vulkan_layer test_ncnn_vulkan_layer.cpp)
    target_include_directories(test_ncnn_vulkan_layer PRIVATE
        ${NCNN_DIR}/src
        ${NCNN_BUILD_DIR}/src
        ${NCNN_DIR}/src/layer
        ${NCNN_DIR}/src/layer/vulkan
    )
    target_link_libraries(test_ncnn_vulkan_layer PRIVATE
        -Wl,--whole-archive glvk_static ${NCNN_LIB} -Wl,--no-whole-archive
        ${NCNN_BUILD_DIR}/glslang/glslang/libglslang.a
        ${NCNN_BUILD_DIR}/glslang/glslang/libMachineIndependent.a
        ${NCNN_BUILD_DIR}/glslang/glslang/libGenericCodeGen.a
        ${NCNN_BUILD_DIR}/glslang/glslang/OSDependent/Unix/libOSDependent.a
        ${NCNN_BUILD_DIR}/glslang/OGLCompilersDLL/libOGLCompiler.a
        ${NCNN_BUILD_DIR}/glslang/SPIRV/libSPIRV.a
        pthread
        dl
        GL
        EGL
    )
    target_compile_options(test_ncnn_vulkan_layer PRIVATE -fopenmp)
    target_link_options(test_ncnn_vulkan_layer PRIVATE -fopenmp)
    add_test(NAME test_ncnn_vulkan_layer COMMAND test_ncnn_vulkan_layer)

    add_executable(test_rife_layers test_rife_layers.cpp)
    target_include_directories(test_rife_layers PRIVATE
        ${NCNN_DIR}/src
        ${NCNN_BUILD_DIR}/src
        ${NCNN_DIR}/src/layer
        ${NCNN_DIR}/src/layer/vulkan
    )
    target_link_libraries(test_rife_layers PRIVATE
        -Wl,--whole-archive glvk_static ${NCNN_LIB} -Wl,--no-whole-archive
        ${NCNN_BUILD_DIR}/glslang/glslang/libglslang.a
        ${NCNN_BUILD_DIR}/glslang/glslang/libMachineIndependent.a
        ${NCNN_BUILD_DIR}/glslang/glslang/libGenericCodeGen.a
        ${NCNN_BUILD_DIR}/glslang/glslang/OSDependent/Unix/libOSDependent.a
        ${NCNN_BUILD_DIR}/glslang/OGLCompilersDLL/libOGLCompiler.a
        ${NCNN_BUILD_DIR}/glslang/SPIRV/libSPIRV.a
        pthread
        dl
        GL
        EGL
    )
    target_compile_options(test_rife_layers PRIVATE -fopenmp)
    target_link_options(test_rife_layers PRIVATE -fopenmp)
    add_test(NAME test_rife_layers COMMAND test_rife_layers)
endif()
