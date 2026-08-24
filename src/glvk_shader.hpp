#pragma once

#include "glvk_internal.hpp"
#include <vector>
#include <cstdint>

GLuint CompileSPIRVToGLProgram(
    const std::vector<uint32_t>& spirv_words,
    const VkSpecializationInfo* spec_info,
    std::vector<GLVKPushConstantUniform>& out_push_uniforms
);
