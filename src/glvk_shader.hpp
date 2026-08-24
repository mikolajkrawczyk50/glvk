#pragma once

#include "glvk_internal.hpp"
#include <spirv_glsl.hpp>

GLuint CompileSpirvToGLProgram(
    const std::vector<uint32_t>& spirv_words,
    const VkSpecializationInfo* spec_info,
    VkPipelineLayout layout,
    std::vector<PushConstantUniformMap>& out_push_uniforms
);
