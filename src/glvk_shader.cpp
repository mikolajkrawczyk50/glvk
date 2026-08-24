#include "glvk_shader.hpp"
#include <iostream>
#include <vector>
#include <cstring>

GLuint CompileSpirvToGLProgram(
    const std::vector<uint32_t>& spirv_words,
    const VkSpecializationInfo* spec_info,
    VkPipelineLayout layout,
    std::vector<PushConstantUniformMap>& out_push_uniforms) {

    try {
        spirv_cross::CompilerGLSL glsl(spirv_words.data(), spirv_words.size());

        spirv_cross::CompilerGLSL::Options options;
        options.version = 430;
        options.es = false;
        options.vulkan_semantics = false;
        options.emit_push_constant_as_uniform_buffer = false;
        options.enable_420pack_extension = true;
        glsl.set_common_options(options);

        // Apply Specialization Constants
        if (spec_info && spec_info->mapEntryCount > 0 && spec_info->pData) {
            auto scs = glsl.get_specialization_constants();
            const uint8_t* spec_data = (const uint8_t*)spec_info->pData;

            for (uint32_t i = 0; i < spec_info->mapEntryCount; i++) {
                const auto& entry = spec_info->pMapEntries[i];
                for (const auto& sc : scs) {
                    if (sc.constant_id == entry.constantID) {
                        auto& c = glsl.get_constant(sc.id);
                        const auto& type = glsl.get_type(c.constant_type);
                        if (type.basetype == spirv_cross::SPIRType::Float) {
                            c.m.c[0].r[0].f32 = *(const float*)(spec_data + entry.offset);
                        } else if (type.basetype == spirv_cross::SPIRType::Int) {
                            c.m.c[0].r[0].i32 = *(const int32_t*)(spec_data + entry.offset);
                        } else {
                            c.m.c[0].r[0].u32 = *(const uint32_t*)(spec_data + entry.offset);
                        }
                    }
                }
            }
        }

        std::string glsl_source = glsl.compile();

        GLuint shader = gl.CreateShader(GL_COMPUTE_SHADER);
        const char* src_cstr = glsl_source.c_str();
        gl.ShaderSource(shader, 1, &src_cstr, nullptr);
        gl.CompileShader(shader);

        GLint status = 0;
        gl.GetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (!status) {
            char log[4096];
            GLsizei len = 0;
            gl.GetShaderInfoLog(shader, sizeof(log), &len, log);
            std::cerr << "[GLVK] Shader compilation failed:\n" << log << "\n--- Source ---\n" << glsl_source << std::endl;
            gl.DeleteShader(shader);
            return 0;
        }

        GLuint program = gl.CreateProgram();
        gl.AttachShader(program, shader);
        gl.LinkProgram(program);

        gl.GetProgramiv(program, GL_LINK_STATUS, &status);
        if (!status) {
            char log[4096];
            GLsizei len = 0;
            gl.GetProgramInfoLog(program, sizeof(log), &len, log);
            std::cerr << "[GLVK] Program link failed:\n" << log << std::endl;
            gl.DeleteProgram(program);
            gl.DeleteShader(shader);
            return 0;
        }

        gl.DetachShader(program, shader);
        gl.DeleteShader(shader);

        // Find push constant uniform locations
        auto resources = glsl.get_shader_resources();
        for (const auto& push_block : resources.push_constant_buffers) {
            const auto& block_type = glsl.get_type(push_block.base_type_id);
            for (size_t i = 0; i < block_type.member_types.size(); i++) {
                std::string member_name = glsl.get_member_name(push_block.base_type_id, (uint32_t)i);
                std::string uniform_name = push_block.name + "." + member_name;
                GLint loc = gl.GetUniformLocation(program, uniform_name.c_str());
                if (loc < 0) {
                    // Try without block name
                    loc = gl.GetUniformLocation(program, member_name.c_str());
                }
                if (loc >= 0) {
                    size_t offset = glsl.type_struct_member_offset(block_type, (uint32_t)i);
                    size_t size = glsl.get_declared_struct_member_size(block_type, (uint32_t)i);
                    out_push_uniforms.push_back({ loc, (uint32_t)offset, (uint32_t)size });
                }
            }
        }

        return program;
    } catch (const std::exception& e) {
        std::cerr << "[GLVK] SPIRV-Cross Exception: " << e.what() << std::endl;
        return 0;
    }
}

extern "C" {

VkResult vkCreateShaderModule(VkDevice device,
                              const VkShaderModuleCreateInfo* pCreateInfo,
                              const VkAllocationCallbacks* pAllocator,
                              VkShaderModule* pShaderModule) {
    if (!device || !pCreateInfo || !pShaderModule) return VK_ERROR_INITIALIZATION_FAILED;

    auto mod = new VkShaderModule_T();
    size_t num_words = pCreateInfo->codeSize / sizeof(uint32_t);
    mod->spirv_words.assign(pCreateInfo->pCode, pCreateInfo->pCode + num_words);

    *pShaderModule = mod;
    return VK_SUCCESS;
}

void vkDestroyShaderModule(VkDevice device,
                           VkShaderModule shaderModule,
                           const VkAllocationCallbacks* pAllocator) {
    if (!shaderModule) return;
    delete shaderModule;
}

} // extern "C"
