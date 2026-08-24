#include "glvk_shader.hpp"
#include "glvk_internal.hpp"
#include <spirv_glsl.hpp>
#include <iostream>
#include <vector>
#include <string>

static void PatchGLSLSource(std::string& src) {
    auto replace_all = [](std::string& s, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    replace_all(src, "#error No extension available for FP16.", "// FP16 mapped to float");
    replace_all(src, "#error No extension available for Int8.", "// Int8 mapped to uint");
    replace_all(src, "#error No extension available for Int16.", "// Int16 mapped to int");

    std::string type_defs = 
        "\n// GLVK FP16 / Int8 emulation macros\n"
        "#ifndef GL_AMD_gpu_shader_half_float\n"
        "#define float16_t float\n"
        "#define f16vec2 vec2\n"
        "#define f16vec3 vec3\n"
        "#define f16vec4 vec4\n"
        "#endif\n"
        "#ifndef GL_EXT_shader_explicit_arithmetic_types_int8\n"
        "#define uint8_t uint\n"
        "#define u8vec2 uvec2\n"
        "#define u8vec3 uvec3\n"
        "#define u8vec4 uvec4\n"
        "#define int8_t int\n"
        "#define i8vec2 ivec2\n"
        "#define i8vec3 ivec3\n"
        "#define i8vec4 ivec4\n"
        "#endif\n"
        "#define uint16_t uint\n"
        "#define u16vec2 uvec2\n"
        "#define u16vec3 uvec3\n"
        "#define u16vec4 uvec4\n"
        "#define int16_t int\n"
        "#define i16vec2 ivec2\n"
        "#define i16vec3 ivec3\n"
        "#define i16vec4 ivec4\n";

    size_t version_pos = src.find("#version 430");
    if (version_pos != std::string::npos) {
        size_t line_end = src.find('\n', version_pos);
        if (line_end != std::string::npos) {
            src.insert(line_end + 1, type_defs);
        }
    }
}

GLuint CompileSPIRVToGLProgram(
    const std::vector<uint32_t>& spirv_words,
    const VkSpecializationInfo* spec_info,
    std::vector<GLVKPushConstantUniform>& out_push_uniforms)
{
    try {
        spirv_cross::CompilerGLSL glsl(spirv_words);

        // Apply Specialization Constants if provided
        if (spec_info && spec_info->mapEntryCount > 0 && spec_info->pData) {
            auto constants = glsl.get_specialization_constants();
            const uint8_t* spec_data = static_cast<const uint8_t*>(spec_info->pData);

            for (uint32_t i = 0; i < spec_info->mapEntryCount; i++) {
                const auto& entry = spec_info->pMapEntries[i];
                for (const auto& sc : constants) {
                    if (sc.constant_id == entry.constantID) {
                        auto& c = glsl.get_constant(sc.id);
                        if (entry.size == sizeof(uint32_t)) {
                            uint32_t val = *reinterpret_cast<const uint32_t*>(spec_data + entry.offset);
                            c.m.c[0].r[0].u32 = val;
                        } else if (entry.size == sizeof(uint64_t)) {
                            uint64_t val = *reinterpret_cast<const uint64_t*>(spec_data + entry.offset);
                            c.m.c[0].r[0].u64 = val;
                        }
                    }
                }
            }
        }

        spirv_cross::CompilerGLSL::Options options;
        options.version = 430;
        options.es = false;
        options.vulkan_semantics = false;
        options.emit_push_constant_as_uniform_buffer = false;
        glsl.set_common_options(options);

        std::string glsl_source = glsl.compile();
        PatchGLSLSource(glsl_source);

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
                size_t offset = glsl.type_struct_member_offset(block_type, (uint32_t)i);
                size_t size = glsl.get_declared_struct_member_size(block_type, (uint32_t)i);
                const auto& member_type = glsl.get_type(block_type.member_types[i]);

                uint32_t type_enum = 0; // 0 = int, 1 = float, 2 = uint
                if (member_type.basetype == spirv_cross::SPIRType::Float) {
                    type_enum = 1;
                } else if (member_type.basetype == spirv_cross::SPIRType::UInt) {
                    type_enum = 2;
                }

                GLint loc = gl.GetUniformLocation(program, member_name.c_str());
                if (loc < 0 && !push_block.name.empty()) {
                    std::string u1 = push_block.name + "." + member_name;
                    loc = gl.GetUniformLocation(program, u1.c_str());
                }
                if (loc < 0 && !push_block.name.empty()) {
                    std::string u2 = push_block.name + "_" + member_name;
                    loc = gl.GetUniformLocation(program, u2.c_str());
                }
                if (loc < 0) {
                    std::string u3 = "_" + member_name;
                    loc = gl.GetUniformLocation(program, u3.c_str());
                }

                if (loc >= 0) {
                    out_push_uniforms.push_back({ loc, (uint32_t)offset, (uint32_t)size, type_enum });
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
