#include "glvk_shader.hpp"
#include "glvk_internal.hpp"
#include <spirv_glsl.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <regex>

static void PatchGLSLSource(std::string& src) {
    auto replace_all = [](std::string& s, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    // Make storage buffers coherent so atomic and sub-dword operations succeed
    replace_all(src, "writeonly buffer", "coherent buffer");
    replace_all(src, "readonly buffer", "buffer");
    replace_all(src, "std430) buffer", "std430) coherent buffer");

    std::vector<std::string> u8_vars;
    std::vector<std::string> i8_vars;
    std::vector<std::string> f16_vars;
    std::vector<std::string> f16vec2_vars;
    std::vector<std::string> f16vec4_vars;

    auto find_subdword_buffers = [&](const std::string& type_name, std::vector<std::string>& out_vars) {
        size_t pos = 0;
        while ((pos = src.find(type_name + " ", pos)) != std::string::npos) {
            size_t name_start = pos + type_name.length() + 1;
            while (name_start < src.size() && (src[name_start] == ' ' || src[name_start] == '\t')) name_start++;
            size_t bracket_pos = src.find("[];", name_start);
            if (bracket_pos != std::string::npos && bracket_pos > name_start) {
                size_t nl = src.find('\n', name_start);
                if (nl == std::string::npos || nl > bracket_pos) {
                    std::string var_name = src.substr(name_start, bracket_pos - name_start);
                    while (!var_name.empty() && isspace(var_name.back())) var_name.pop_back();
                    if (!var_name.empty()) {
                        bool found = false;
                        for (const auto& v : out_vars) { if (v == var_name) { found = true; break; } }
                        if (!found) out_vars.push_back(var_name);
                    }
                }
            }
            pos = name_start;
        }
    };











    find_subdword_buffers("uint8_t", u8_vars);
    find_subdword_buffers("int8_t", i8_vars);
    find_subdword_buffers("float16_t", f16_vars);

    bool needs_subdword = !u8_vars.empty() || !i8_vars.empty() || !f16_vars.empty();

    // Replace SSBO declarations from sub-dword types to uint
    for (const auto& v : u8_vars) {
        replace_all(src, "uint8_t " + v + "[];", "uint " + v + "[];");
    }
    for (const auto& v : i8_vars) {
        replace_all(src, "int8_t " + v + "[];", "uint " + v + "[];");
    }
    for (const auto& v : f16_vars) {
        replace_all(src, "float16_t " + v + "[];", "uint " + v + "[];");
    }

    // Remove FP16/Int8/Int16 extension error directives
    replace_all(src, "#error No extension available for FP16.", "// FP16 mapped to float");
    replace_all(src, "#error No extension available for Int8.", "// Int8 mapped to uint");
    replace_all(src, "#error No extension available for Int16.", "// Int16 mapped to int");
    replace_all(src, ": require", ": enable");

    // Insert type-widening macros and sub-dword helper macros after #version line
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
        "#define i16vec4 ivec4\n"
        "\n// GLVK sub-dword emulated packing/unpacking helpers (pure IEEE-754 bitwise)\n"
        "float glvk_half_to_float(uint h) {\n"
        "    uint sign = (h & 0x8000u) << 16;\n"
        "    uint exp = (h & 0x7C00u) >> 10;\n"
        "    uint mant = (h & 0x03FFu);\n"
        "    if (exp == 0u) {\n"
        "        if (mant == 0u) return uintBitsToFloat(sign);\n"
        "        while ((mant & 0x0400u) == 0u) { mant <<= 1; exp--; }\n"
        "        exp++; mant &= 0x03FFu;\n"
        "        return uintBitsToFloat(sign | ((exp + 112u) << 23) | (mant << 13));\n"
        "    } else if (exp == 31u) {\n"
        "        return uintBitsToFloat(sign | 0x7F800000u | (mant << 13));\n"
        "    }\n"
        "    return uintBitsToFloat(sign | ((exp + 112u) << 23) | (mant << 13));\n"
        "}\n\n"
        "uint glvk_float_to_half(float f) {\n"
        "    uint u = floatBitsToUint(f);\n"
        "    uint sign = (u >> 16) & 0x8000u;\n"
        "    int exp = int((u >> 23) & 0xFFu) - 127 + 15;\n"
        "    uint mant = u & 0x007FFFFFu;\n"
        "    if (exp <= 0) {\n"
        "        if (exp < -10) return sign;\n"
        "        mant = (mant | 0x00800000u) >> (1 - exp);\n"
        "        return sign | ((mant + 0x00001000u) >> 13);\n"
        "    } else if (exp >= 31) {\n"
        "        return sign | 0x7C00u;\n"
        "    }\n"
        "    return sign | (uint(exp) << 10) | ((mant + 0x00001000u) >> 13);\n"
        "}\n\n"
        "vec2 glvk_unpackHalf2x16(uint u) {\n"
        "    return vec2(glvk_half_to_float(u & 0xFFFFu), glvk_half_to_float(u >> 16));\n"
        "}\n"
        "uint glvk_packHalf2x16(vec2 v) {\n"
        "    return (glvk_float_to_half(v.x) & 0xFFFFu) | (glvk_float_to_half(v.y) << 16);\n"
        "}\n"
        "#define unpackHalf2x16 glvk_unpackHalf2x16\n"
        "#define packHalf2x16 glvk_packHalf2x16\n";

    std::string helpers;
    if (needs_subdword) {
        helpers =
            "#define glvk_write_u8(buf, idx, val) do { \\\n"
            "    int _w = int(idx) >> 2; \\\n"
            "    int _shift = (int(idx) & 3) * 8; \\\n"
            "    atomicAnd(buf[_w], ~(0xFFu << _shift)); \\\n"
            "    atomicOr(buf[_w], (uint(val) & 0xFFu) << _shift); \\\n"
            "} while(false)\n\n"
            "#define glvk_write_i8(buf, idx, val) glvk_write_u8(buf, idx, val)\n"
            "#define glvk_read_u8(buf, idx) ((buf[int(idx) >> 2] >> ((int(idx) & 3) * 8)) & 0xFFu)\n"
            "#define glvk_read_i8(buf, idx) (int((buf[int(idx) >> 2] >> ((int(idx) & 3) * 8)) & 0xFFu))\n\n"
            "#define glvk_write_f16(buf, idx, val) do { \\\n"
            "    int _w = int(idx) >> 1; \\\n"
            "    int _shift = (int(idx) & 1) * 16; \\\n"
            "    atomicAnd(buf[_w], ~(0xFFFFu << _shift)); \\\n"
            "    atomicOr(buf[_w], (glvk_float_to_half(float(val)) & 0xFFFFu) << _shift); \\\n"
            "} while(false)\n\n"
            "#define glvk_read_f16(buf, idx) (glvk_half_to_float((buf[int(idx) >> 1] >> (((int(idx) & 1)) * 16)) & 0xFFFFu))\n\n"
            "#define glvk_write_f16vec2(buf, idx, val) (buf[int(idx)] = (glvk_float_to_half((val).x) & 0xFFFFu) | (glvk_float_to_half((val).y) << 16))\n"
            "#define glvk_read_f16vec2(buf, idx) vec2(glvk_half_to_float(buf[int(idx)] & 0xFFFFu), glvk_half_to_float(buf[int(idx)] >> 16))\n\n"
            "#define glvk_write_f16vec4(buf, idx, val) (buf[int(idx)] = uvec2((glvk_float_to_half((val).x) & 0xFFFFu) | (glvk_float_to_half((val).y) << 16), (glvk_float_to_half((val).z) & 0xFFFFu) | (glvk_float_to_half((val).w) << 16)))\n"
            "#define glvk_read_f16vec4(buf, idx) vec4(glvk_half_to_float(buf[int(idx)].x & 0xFFFFu), glvk_half_to_float(buf[int(idx)].x >> 16), glvk_half_to_float(buf[int(idx)].y & 0xFFFFu), glvk_half_to_float(buf[int(idx)].y >> 16))\n\n";
    }

    size_t version_pos = src.find("#version 430");
    if (version_pos != std::string::npos) {
        size_t line_end = src.find('\n', version_pos);
        if (line_end != std::string::npos) {
            src.insert(line_end + 1, type_defs + helpers);
        }
    }

    // Helper to find matching ']' accounting for nested brackets
    auto find_matching_bracket = [](const std::string& s, size_t open_pos) -> size_t {
        int depth = 1;
        for (size_t i = open_pos + 1; i < s.size(); i++) {
            if (s[i] == '[') depth++;
            else if (s[i] == ']') { depth--; if (depth == 0) return i; }
        }
        return std::string::npos;
    };

    // Helper: find start of identifier before a '.' (SPIRV-Cross generates _NNN identifiers)
    auto find_ident_start = [](const std::string& s, size_t dot_pos) -> size_t {
        if (dot_pos == 0) return std::string::npos;
        size_t p = dot_pos - 1;
        while (p > 0 && (isalnum(s[p]) || s[p] == '_')) p--;
        if (!(isalnum(s[p]) || s[p] == '_')) p++;
        return p;
    };

    auto patch_writes = [&](const std::string& var_name, const std::string& write_macro, const std::string& val_type) {
        std::string pattern = "." + var_name + "[";
        size_t pos = 0;
        while ((pos = src.find(pattern, pos)) != std::string::npos) {
            size_t dot_pos = pos;
            size_t id_start = find_ident_start(src, dot_pos);
            if (id_start == std::string::npos) { pos++; continue; }

            std::string ident = src.substr(id_start, dot_pos - id_start);
            size_t bracket_open = dot_pos + pattern.length();
            size_t bracket_close = find_matching_bracket(src, bracket_open - 1);
            if (bracket_close == std::string::npos) { pos++; continue; }

            // Check if write (next non-space char is '=' or '.comp =')
            size_t after_bracket = bracket_close + 1;
            while (after_bracket < src.size() && src[after_bracket] == ' ') after_bracket++;
            if (after_bracket >= src.size()) { pos++; continue; }

            int comp_offset = -1;
            if (src[after_bracket] == '.' && after_bracket + 1 < src.size()) {
                char comp = src[after_bracket + 1];
                if (comp == 'x' || comp == 'r') comp_offset = 0;
                else if (comp == 'y' || comp == 'g') comp_offset = 1;
                else if (comp == 'z' || comp == 'b') comp_offset = 2;
                else if (comp == 'w' || comp == 'a') comp_offset = 3;
                after_bracket += 2;
                while (after_bracket < src.size() && src[after_bracket] == ' ') after_bracket++;
            }

            if (after_bracket >= src.size() || src[after_bracket] != '=') { pos++; continue; }

            size_t semi_pos = src.find(';', after_bracket);
            if (semi_pos == std::string::npos) { pos++; continue; }

            std::string idx = src.substr(bracket_open, bracket_close - bracket_open);
            std::string expr = src.substr(after_bracket + 1, semi_pos - (after_bracket + 1));
            while (!expr.empty() && (expr.front() == ' ' || expr.front() == '\t')) expr.erase(0, 1);
            while (!expr.empty() && (expr.back() == ' ' || expr.back() == '\t')) expr.pop_back();

            if (expr.rfind("f16vec4(", 0) == 0 && expr.back() == ')') expr = expr.substr(8, expr.length() - 9);
            if (expr.rfind("vec4(", 0) == 0 && expr.back() == ')') expr = expr.substr(5, expr.length() - 6);
            if (expr.rfind("f16vec2(", 0) == 0 && expr.back() == ')') expr = expr.substr(8, expr.length() - 9);
            if (expr.rfind("vec2(", 0) == 0 && expr.back() == ')') expr = expr.substr(5, expr.length() - 6);
            if (expr.rfind("float16_t(", 0) == 0 && expr.back() == ')') expr = expr.substr(10, expr.length() - 11);
            if (expr.rfind("float(", 0) == 0 && expr.back() == ')') expr = expr.substr(6, expr.length() - 7);
            if (expr.rfind("uint8_t(", 0) == 0 && expr.back() == ')') expr = expr.substr(8, expr.length() - 9);
            if (expr.rfind("uint(", 0) == 0 && expr.back() == ')') expr = expr.substr(5, expr.length() - 6);
            if (expr.rfind("int8_t(", 0) == 0 && expr.back() == ')') expr = expr.substr(7, expr.length() - 8);
            if (expr.rfind("int(", 0) == 0 && expr.back() == ')') expr = expr.substr(4, expr.length() - 5);

            std::string rep;
            if (comp_offset >= 0) {
                int vec_size = (val_type == "vec4" || val_type == "f16vec4") ? 4 : 2;
                rep = "glvk_write_f16(" + ident + "." + var_name + ", int(" + idx + ") * " + std::to_string(vec_size) + " + " + std::to_string(comp_offset) + ", float(" + expr + "));";
            } else {
                rep = write_macro + "(" + ident + "." + var_name + ", int(" + idx + "), " + val_type + "(" + expr + "));";
            }
            src.replace(id_start, (semi_pos + 1) - id_start, rep);
            pos = id_start + rep.length();
        }
    };

    auto patch_reads = [&](const std::string& var_name, const std::string& read_macro) {
        std::string pattern = "." + var_name + "[";
        size_t pos = 0;
        while ((pos = src.find(pattern, pos)) != std::string::npos) {
            size_t dot_pos = pos;
            size_t id_start = find_ident_start(src, dot_pos);
            if (id_start == std::string::npos) { pos++; continue; }

            std::string ident = src.substr(id_start, dot_pos - id_start);
            size_t bracket_open = dot_pos + pattern.length();
            size_t bracket_close = find_matching_bracket(src, bracket_open - 1);
            if (bracket_close == std::string::npos) { pos++; continue; }

            std::string idx = src.substr(bracket_open, bracket_close - bracket_open);
            std::string rep = read_macro + "(" + ident + "." + var_name + ", " + idx + ")";
            src.replace(id_start, (bracket_close + 1) - id_start, rep);
            pos = id_start + rep.length();
        }
    };

    // Process WRITES first, then READS for each sub-dword buffer
    for (const auto& v : u8_vars) {
        patch_writes(v, "glvk_write_u8", "uint");
        patch_reads(v, "glvk_read_u8");
    }
    for (const auto& v : i8_vars) {
        patch_writes(v, "glvk_write_i8", "int");
        patch_reads(v, "glvk_read_i8");
    }
    for (const auto& v : f16_vars) {
        patch_writes(v, "glvk_write_f16", "float");
        patch_reads(v, "glvk_read_f16");
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
                            c.specialization = false;
                        } else if (entry.size == sizeof(uint64_t)) {
                            uint64_t val = *reinterpret_cast<const uint64_t*>(spec_data + entry.offset);
                            c.m.c[0].r[0].u64 = val;
                            c.specialization = false;
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
        options.enable_storage_image_qualifier_deduction = false;
        glsl.set_common_options(options);

        std::cerr << "[GLVK] Starting SPIR-V to GLSL compile..." << std::endl;

        // Remap (set, binding) pairs to a flat binding namespace for OpenGL.
        // On Fermi GF108 / GT 730, GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS is limited (8 to 16).
        // Using set * 4 + binding keeps set 0 (0..3), set 1 (4..7), set 2 (8..11), set 3 (12..15)
        // safely within hardware limits without overflowing SSBO binding tables.
        {
            auto remap_resources = [&](const auto& resources) {
                for (const auto& res : resources) {
                    uint32_t set = glsl.get_decoration(res.id, spv::DecorationDescriptorSet);
                    uint32_t binding = glsl.get_decoration(res.id, spv::DecorationBinding);
                    uint32_t new_binding = set * 4 + binding;
                    glsl.set_decoration(res.id, spv::DecorationBinding, new_binding);
                }
            };
            auto shader_resources = glsl.get_shader_resources();
            remap_resources(shader_resources.storage_buffers);
            remap_resources(shader_resources.uniform_buffers);
            remap_resources(shader_resources.storage_images);
            remap_resources(shader_resources.sampled_images);
            remap_resources(shader_resources.separate_images);
            remap_resources(shader_resources.separate_samplers);
        }

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

        if (GetGLVKLogLevel() >= 2) {
            std::string fname = "/tmp/glvk_prog_" + std::to_string(program) + ".glsl";
            FILE* fp = fopen(fname.c_str(), "w");
            if (fp) {
                fwrite(glsl_source.c_str(), 1, glsl_source.size(), fp);
                fclose(fp);
            }
            std::string spv_name = "/tmp/glvk_prog_" + std::to_string(program) + ".spv";
            FILE* sfp = fopen(spv_name.c_str(), "wb");
            if (sfp) {
                fwrite(spirv_words.data(), sizeof(uint32_t), spirv_words.size(), sfp);
                fclose(sfp);
            }
        }

        // Find push constant uniform locations
        auto resources = glsl.get_shader_resources();
        for (const auto& push_block : resources.push_constant_buffers) {
            const auto& block_type = glsl.get_type(push_block.base_type_id);
            std::string var_name = glsl.get_name(push_block.id);
            for (size_t i = 0; i < block_type.member_types.size(); i++) {
                std::string member_name = glsl.get_member_name(push_block.base_type_id, (uint32_t)i);
                size_t offset = glsl.type_struct_member_offset(block_type, (uint32_t)i);
                size_t size = glsl.get_declared_struct_member_size(block_type, (uint32_t)i);
                const auto& member_type = glsl.get_type(block_type.member_types[i]);

                uint32_t type_enum = 0; // 0 = int/bool, 1 = float, 2 = uint
                if (member_type.basetype == spirv_cross::SPIRType::Float ||
                    member_type.basetype == spirv_cross::SPIRType::Half) {
                    type_enum = 1;
                } else if (member_type.basetype == spirv_cross::SPIRType::UInt ||
                           member_type.basetype == spirv_cross::SPIRType::UByte ||
                           member_type.basetype == spirv_cross::SPIRType::UShort) {
                    type_enum = 2;
                } else {
                    type_enum = 0;
                }

                // Try various naming conventions used by GLSL compilers
                std::vector<std::string> candidates;
                if (!var_name.empty()) candidates.push_back(var_name + "." + member_name);
                candidates.push_back("p." + member_name);
                candidates.push_back(member_name);
                if (!push_block.name.empty()) {
                    candidates.push_back(push_block.name + "." + member_name);
                    candidates.push_back(push_block.name + "_" + member_name);
                }
                candidates.push_back("_" + member_name);

                GLint loc = -1;
                for (const auto& name : candidates) {
                    loc = gl.GetUniformLocation(program, name.c_str());
                    if (loc >= 0) break;
                }

                if (loc >= 0) {
                    out_push_uniforms.push_back({ loc, (uint32_t)offset, (uint32_t)size, type_enum });
                }
            }
        }
        if (GetGLVKLogLevel() >= 2) {
            std::cerr << "[GLVK PROG " << program << "] push uniforms count = " << out_push_uniforms.size() << std::endl;
            for (const auto& u : out_push_uniforms) {
                std::cerr << "  -> loc=" << u.location << " offset=" << u.offset << " size=" << u.size << " type=" << u.type << std::endl;
            }
        }

        return program;
    } catch (const std::exception& e) {
        if (GetGLVKLogLevel() >= 1) {
            std::cerr << "[GLVK] SPIRV-Cross Exception: " << e.what() << std::endl;
        }
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
