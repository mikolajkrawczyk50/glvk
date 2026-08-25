add_executable(test_vector_add test_vector_add.cpp)
target_link_libraries(test_vector_add PRIVATE glvk_static)
add_test(NAME test_vector_add COMMAND test_vector_add)

add_executable(test_spec_constant test_spec_constant.cpp)
target_link_libraries(test_spec_constant PRIVATE glvk_static)
add_test(NAME test_spec_constant COMMAND test_spec_constant)

if(EXISTS "/tmp/ncnn_build/src/libncnn.a")
    add_executable(test_ncnn_vulkan_layer test_ncnn_vulkan_layer.cpp)
    target_include_directories(test_ncnn_vulkan_layer PRIVATE
        /tmp/ncnn/src
        /tmp/ncnn_build/src
        /tmp/ncnn/src/layer
        /tmp/ncnn/src/layer/vulkan
    )
    target_link_libraries(test_ncnn_vulkan_layer PRIVATE
        vulkan
        /tmp/ncnn_build/src/libncnn.a
        glslang
        SPIRV
        glslang-default-resource-limits
        pthread
        dl
        GL
        EGL
    )
    target_compile_options(test_ncnn_vulkan_layer PRIVATE -fopenmp)
    target_link_options(test_ncnn_vulkan_layer PRIVATE -fopenmp)
    add_test(NAME test_ncnn_vulkan_layer COMMAND test_ncnn_vulkan_layer)

    add_executable(test_rife_model test_rife_model.cpp)
    target_include_directories(test_rife_model PRIVATE
        /tmp/ncnn/src
        /tmp/ncnn_build/src
        /tmp/ncnn/src/layer
        /tmp/ncnn/src/layer/vulkan
    )
    target_link_libraries(test_rife_model PRIVATE
        vulkan
        /tmp/ncnn_build/src/libncnn.a
        glslang
        SPIRV
        glslang-default-resource-limits
        pthread
        dl
        GL
        EGL
    )
    target_compile_options(test_rife_model PRIVATE -fopenmp)
    target_link_options(test_rife_model PRIVATE -fopenmp)
    add_test(NAME test_rife_model COMMAND test_rife_model)

    add_executable(test_rife_layers test_rife_layers.cpp)
    target_include_directories(test_rife_layers PRIVATE
        /tmp/ncnn/src
        /tmp/ncnn_build/src
        /tmp/ncnn/src/layer
        /tmp/ncnn/src/layer/vulkan
    )
    target_link_libraries(test_rife_layers PRIVATE
        vulkan
        /tmp/ncnn_build/src/libncnn.a
        glslang
        SPIRV
        glslang-default-resource-limits
        pthread
        dl
        GL
        EGL
    )
    target_compile_options(test_rife_layers PRIVATE -fopenmp)
    target_link_options(test_rife_layers PRIVATE -fopenmp)
    add_test(NAME test_rife_layers COMMAND test_rife_layers)
endif()
