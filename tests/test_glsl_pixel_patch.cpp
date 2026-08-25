#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include "gl_backend.hpp"

// Standalone copy of PatchGLSLSource for testing
static void PatchGLSLSource(std::string& src) {
    auto replace_all = [](std::string& s, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    replace_all(src, "writeonly buffer", "buffer");
    replace_all(src, "readonly buffer", "buffer");

    bool has_u8_bottom = (src.find("uint8_t bottom_blob_data[];") != std::string::npos);
    bool has_f16_top = (src.find("float16_t top_blob_data[];") != std::string::npos);
    bool has_f16_bottom = (src.find("float16_t bottom_blob_data[];") != std::string::npos);
    bool has_u8_top = (src.find("uint8_t top_blob_data[];") != std::string::npos);
    bool needs_subdword = has_u8_bottom || has_f16_top || has_f16_bottom || has_u8_top;

    if (needs_subdword) {
        replace_all(src, "uint8_t bottom_blob_data[];", "uint bottom_blob_data[];");
        replace_all(src, "float16_t top_blob_data[];", "uint top_blob_data[];");
        replace_all(src, "float16_t bottom_blob_data[];", "uint bottom_blob_data[];");
        replace_all(src, "uint8_t top_blob_data[];", "uint top_blob_data[];");
    }

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

    std::string helpers;
    if (needs_subdword) {
        helpers =
            "\n// GLVK sub-dword emulated packing/unpacking helpers\n"
            "#define glvk_write_f16(buf, idx, val) do { \\\n"
            "    int _w = int(idx) >> 1; \\\n"
            "    int _shift = (int(idx) & 1) * 16; \\\n"
            "    uint _h = packHalf2x16(vec2(val, 0.0)) & 0xFFFFu; \\\n"
            "    for (;;) { \\\n"
            "        uint _old_val = buf[_w]; \\\n"
            "        uint _new_val = (_old_val & ~(0xFFFFu << _shift)) | (_h << _shift); \\\n"
            "        if (atomicCompSwap(buf[_w], _old_val, _new_val) == _old_val) break; \\\n"
            "    } \\\n"
            "} while(false)\n\n"
            "#define glvk_write_u8(buf, idx, val) do { \\\n"
            "    int _w = int(idx) >> 2; \\\n"
            "    int _shift = (int(idx) & 3) * 8; \\\n"
            "    uint _b = uint(val) & 0xFFu; \\\n"
            "    for (;;) { \\\n"
            "        uint _old_val = buf[_w]; \\\n"
            "        uint _new_val = (_old_val & ~(0xFFu << _shift)) | (_b << _shift); \\\n"
            "        if (atomicCompSwap(buf[_w], _old_val, _new_val) == _old_val) break; \\\n"
            "    } \\\n"
            "} while(false)\n\n"
            "#define glvk_read_u8(buf, idx) ((buf[int(idx) >> 2] >> ((int(idx) & 3) * 8)) & 0xFFu)\n"
            "#define glvk_read_f16(buf, idx) (unpackHalf2x16(buf[int(idx) >> 1])[int(idx) & 1])\n\n";
    }

    size_t version_pos = src.find("#version 430");
    if (version_pos != std::string::npos) {
        size_t line_end = src.find('\n', version_pos);
        if (line_end != std::string::npos) {
            src.insert(line_end + 1, type_defs + helpers);
        }
    }

    auto find_matching_bracket = [](const std::string& s, size_t open_pos) -> size_t {
        int depth = 1;
        for (size_t i = open_pos + 1; i < s.size(); i++) {
            if (s[i] == '[') depth++;
            else if (s[i] == ']') { depth--; if (depth == 0) return i; }
        }
        return std::string::npos;
    };

    auto find_ident_start = [](const std::string& s, size_t dot_pos) -> size_t {
        if (dot_pos == 0) return std::string::npos;
        size_t p = dot_pos - 1;
        while (p > 0 && (isalnum(s[p]) || s[p] == '_')) p--;
        if (!(isalnum(s[p]) || s[p] == '_')) p++;
        return p;
    };

    if (has_f16_top) {
        size_t pos = 0;
        while ((pos = src.find(".top_blob_data[", pos)) != std::string::npos) {
            size_t dot_pos = pos;
            size_t id_start = find_ident_start(src, dot_pos);
            if (id_start == std::string::npos) { pos++; continue; }

            std::string ident = src.substr(id_start, dot_pos - id_start);
            size_t bracket_open = dot_pos + strlen(".top_blob_data[");
            size_t bracket_close = find_matching_bracket(src, bracket_open - 1);
            if (bracket_close == std::string::npos) { pos++; continue; }

            size_t after_bracket = bracket_close + 1;
            while (after_bracket < src.size() && src[after_bracket] == ' ') after_bracket++;
            if (after_bracket >= src.size() || src[after_bracket] != '=') { pos++; continue; }

            size_t semi_pos = src.find(';', after_bracket);
            if (semi_pos == std::string::npos) { pos++; continue; }

            std::string idx = src.substr(bracket_open, bracket_close - bracket_open);
            std::string expr = src.substr(after_bracket + 1, semi_pos - (after_bracket + 1));
            while (!expr.empty() && (expr.front() == ' ' || expr.front() == '\t')) expr.erase(0, 1);
            while (!expr.empty() && (expr.back() == ' ' || expr.back() == '\t')) expr.pop_back();

            if (expr.rfind("float16_t(", 0) == 0 && expr.back() == ')') {
                expr = expr.substr(10, expr.length() - 11);
            }
            if (expr.rfind("float(", 0) == 0 && expr.back() == ')') {
                expr = expr.substr(6, expr.length() - 7);
            }

            std::string rep = "glvk_write_f16(" + ident + ".top_blob_data, int(" + idx + "), float(" + expr + "));";
            src.replace(id_start, (semi_pos + 1) - id_start, rep);
            pos = id_start + rep.length();
        }
    }

    if (has_u8_top) {
        size_t pos = 0;
        while ((pos = src.find(".top_blob_data[", pos)) != std::string::npos) {
            size_t dot_pos = pos;
            size_t id_start = find_ident_start(src, dot_pos);
            if (id_start == std::string::npos) { pos++; continue; }

            std::string ident = src.substr(id_start, dot_pos - id_start);
            size_t bracket_open = dot_pos + strlen(".top_blob_data[");
            size_t bracket_close = find_matching_bracket(src, bracket_open - 1);
            if (bracket_close == std::string::npos) { pos++; continue; }

            size_t after_bracket = bracket_close + 1;
            while (after_bracket < src.size() && src[after_bracket] == ' ') after_bracket++;
            if (after_bracket >= src.size() || src[after_bracket] != '=') { pos++; continue; }

            size_t semi_pos = src.find(';', after_bracket);
            if (semi_pos == std::string::npos) { pos++; continue; }

            std::string idx = src.substr(bracket_open, bracket_close - bracket_open);
            std::string expr = src.substr(after_bracket + 1, semi_pos - (after_bracket + 1));
            while (!expr.empty() && (expr.front() == ' ' || expr.front() == '\t')) expr.erase(0, 1);
            while (!expr.empty() && (expr.back() == ' ' || expr.back() == '\t')) expr.pop_back();

            if (expr.rfind("uint8_t(", 0) == 0 && expr.back() == ')') {
                expr = expr.substr(8, expr.length() - 9);
            }
            if (expr.rfind("uint(", 0) == 0 && expr.back() == ')') {
                expr = expr.substr(5, expr.length() - 6);
            }

            std::string rep = "glvk_write_u8(" + ident + ".top_blob_data, int(" + idx + "), uint(" + expr + "));";
            src.replace(id_start, (semi_pos + 1) - id_start, rep);
            pos = id_start + rep.length();
        }
    }

    if (has_u8_bottom) {
        size_t pos = 0;
        while ((pos = src.find(".bottom_blob_data[", pos)) != std::string::npos) {
            size_t dot_pos = pos;
            size_t id_start = find_ident_start(src, dot_pos);
            if (id_start == std::string::npos) { pos++; continue; }

            std::string ident = src.substr(id_start, dot_pos - id_start);
            size_t bracket_open = dot_pos + strlen(".bottom_blob_data[");
            size_t bracket_close = find_matching_bracket(src, bracket_open - 1);
            if (bracket_close == std::string::npos) { pos++; continue; }

            std::string idx = src.substr(bracket_open, bracket_close - bracket_open);
            std::string rep = "glvk_read_u8(" + ident + ".bottom_blob_data, " + idx + ")";
            src.replace(id_start, (bracket_close + 1) - id_start, rep);
            pos = id_start + rep.length();
        }
    }

    if (has_f16_bottom) {
        size_t pos = 0;
        while ((pos = src.find(".bottom_blob_data[", pos)) != std::string::npos) {
            size_t dot_pos = pos;
            size_t id_start = find_ident_start(src, dot_pos);
            if (id_start == std::string::npos) { pos++; continue; }

            std::string ident = src.substr(id_start, dot_pos - id_start);
            size_t bracket_open = dot_pos + strlen(".bottom_blob_data[");
            size_t bracket_close = find_matching_bracket(src, bracket_open - 1);
            if (bracket_close == std::string::npos) { pos++; continue; }

            std::string idx = src.substr(bracket_open, bracket_close - bracket_open);
            std::string rep = "glvk_read_f16(" + ident + ".bottom_blob_data, " + idx + ")";
            src.replace(id_start, (bracket_close + 1) - id_start, rep);
            pos = id_start + rep.length();
        }
    }
}

int main() {
    if (!GLBackend::Instance().Initialize()) {
        std::cerr << "Failed to init GLBackend" << std::endl;
        return 1;
    }
    GLVKContextScope scope;

    // from_pixels: u8 bottom (read), f16 top (write)
    std::string raw_from_pixels =
R"(#version 430
#if defined(GL_AMD_gpu_shader_half_float)
#extension GL_AMD_gpu_shader_half_float : require
#elif defined(GL_NV_gpu_shader5)
#extension GL_NV_gpu_shader5 : require
#else
#error No extension available for FP16.
#endif
#if defined(GL_EXT_shader_explicit_arithmetic_types_int8)
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#elif defined(GL_NV_gpu_shader5)
#extension GL_NV_gpu_shader5 : require
#else
#error No extension available for Int8.
#endif
layout(local_size_x = 8, local_size_y = 8, local_size_z = 3) in;

#ifndef SPIRV_CROSS_CONSTANT_ID_0
#define SPIRV_CROSS_CONSTANT_ID_0 0
#endif
const int bgr = SPIRV_CROSS_CONSTANT_ID_0;
const bool _109 = (bgr == 0);

layout(binding = 1, std430) writeonly buffer top_blob
{
    float16_t top_blob_data[];
} _82;

layout(binding = 0, std430) readonly buffer bottom_blob
{
    uint8_t bottom_blob_data[];
} _118;

struct parameter
{
    int w;
    int h;
    int cstep;
    int outw;
    int outh;
    int outcstep;
};

uniform parameter p;

void main()
{
    int gx = int(gl_GlobalInvocationID.x);
    int gy = int(gl_GlobalInvocationID.y);
    int gz = int(gl_GlobalInvocationID.z);
    if (gx >= p.outw || gy >= p.outh || gz >= 3)
    {
        return;
    }
    if (gx < 0 || gx >= p.w || gy < 0 || gy >= p.h)
    {
        _82.top_blob_data[((gz * p.outcstep) + (gy * p.outw)) + gx] = float16_t(0.0);
        return;
    }
    int v_offset = (gy * p.w) + gx;
    float v;
    if (_109)
    {
        v = float(uint(_118.bottom_blob_data[(v_offset * 3) + gz]));
    }
    else
    {
        v = float(uint(_118.bottom_blob_data[((v_offset * 3) + 2) - gz]));
    }
    _82.top_blob_data[((gz * p.outcstep) + (gy * p.outw)) + gx] = float16_t(v * 0.0039215688593685626983642578125);
}
)";

    // to_pixels: f16 bottom (read), u8 top (write)
    std::string raw_to_pixels =
R"(#version 430
#if defined(GL_AMD_gpu_shader_half_float)
#extension GL_AMD_gpu_shader_half_float : require
#elif defined(GL_NV_gpu_shader5)
#extension GL_NV_gpu_shader5 : require
#else
#error No extension available for FP16.
#endif
#if defined(GL_EXT_shader_explicit_arithmetic_types_int8)
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#elif defined(GL_NV_gpu_shader5)
#extension GL_NV_gpu_shader5 : require
#else
#error No extension available for Int8.
#endif
layout(local_size_x = 8, local_size_y = 8, local_size_z = 3) in;

#ifndef SPIRV_CROSS_CONSTANT_ID_0
#define SPIRV_CROSS_CONSTANT_ID_0 0
#endif
const int bgr = SPIRV_CROSS_CONSTANT_ID_0;
const bool _98 = (bgr == 0);

layout(binding = 0, std430) readonly buffer bottom_blob
{
    float16_t bottom_blob_data[];
} _60;

layout(binding = 1, std430) writeonly buffer top_blob
{
    uint8_t top_blob_data[];
} _105;

struct parameter
{
    int w;
    int h;
    int cstep;
    int outw;
    int outh;
    int outcstep;
};

uniform parameter p;

void main()
{
    int gx = int(gl_GlobalInvocationID.x);
    int gy = int(gl_GlobalInvocationID.y);
    int gz = int(gl_GlobalInvocationID.z);
    if (gx >= p.outw || gy >= p.outh || gz >= 3)
    {
        return;
    }
    float v = float(_60.bottom_blob_data[((gz * p.cstep) + (gy * p.w)) + gx]);
    v = (v * 255.0) + 0.5;
    int v_offset = (gy * p.outw) + gx;
    uint v32 = clamp(uint(floor(v)), 0u, 255u);
    if (_98)
    {
        _105.top_blob_data[(v_offset * 3) + gz] = uint8_t(v32);
    }
    else
    {
        _105.top_blob_data[((v_offset * 3) + 2) - gz] = uint8_t(v32);
    }
}
)";

    // fill_timestamp: f16 top (write only)
    std::string raw_fill_ts =
R"(#version 430
#if defined(GL_AMD_gpu_shader_half_float)
#extension GL_AMD_gpu_shader_half_float : require
#elif defined(GL_NV_gpu_shader5)
#extension GL_NV_gpu_shader5 : require
#else
#error No extension available for FP16.
#endif
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, std430) writeonly buffer top_blob
{
    float16_t top_blob_data[];
} _58;

struct parameter
{
    int w;
    int h;
    int cstep;
    float timestamp;
};

uniform parameter p;

void main()
{
    int gx = int(gl_GlobalInvocationID.x);
    int gy = int(gl_GlobalInvocationID.y);
    int gz = int(gl_GlobalInvocationID.z);
    if (gx >= p.w || gy >= p.h || gz >= 1)
    {
        return;
    }
    _58.top_blob_data[((gz * p.cstep) + (gy * p.w)) + gx] = float16_t(p.timestamp);
}
)";

    auto test_patch_and_compile = [](const std::string& name, std::string src) {
        PatchGLSLSource(src);
        GLuint shader = gl.CreateShader(GL_COMPUTE_SHADER);
        const char* cstr = src.c_str();
        gl.ShaderSource(shader, 1, &cstr, nullptr);
        gl.CompileShader(shader);
        GLint status = 0;
        gl.GetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (!status) {
            char log[4096];
            GLsizei len = 0;
            gl.GetShaderInfoLog(shader, sizeof(log), &len, log);
            std::cerr << "[FAIL] " << name << " compile error:\n" << log << "\n--- Patched Source ---\n" << src << std::endl;
            return false;
        }
        std::cout << "[PASS] " << name << " compiled successfully!" << std::endl;
        gl.DeleteShader(shader);
        return true;
    };

    bool ok = true;
    ok &= test_patch_and_compile("from_pixels", raw_from_pixels);
    ok &= test_patch_and_compile("to_pixels", raw_to_pixels);
    ok &= test_patch_and_compile("fill_timestamp", raw_fill_ts);

    return ok ? 0 : 1;
}
