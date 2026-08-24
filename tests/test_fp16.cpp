#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include "gl_backend.hpp"

int main() {
    if (!GLBackend::Instance().Initialize()) {
        std::cerr << "Failed to init GLBackend" << std::endl;
        return 1;
    }

    std::string glsl = R"(#version 430
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
    bool _37 = gx >= p.outw;
    bool _46;
    if (!_37)
    {
        _46 = gy >= p.outh;
    }
    else
    {
        _46 = _37;
    }
    if (_46 || (gz >= 3))
    {
        return;
    }
    bool _55 = gx < 0;
    bool _63;
    if (!_55)
    {
        _63 = gx >= p.w;
    }
    else
    {
        _63 = _55;
    }
    bool _66 = _63 || (gy < 0);
    bool _75;
    if (!_66)
    {
        _75 = gy >= p.h;
    }
    else
    {
        _75 = _66;
    }
    if (_75)
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

    // Apply patch:
    // 1. Remove #error No extension available
    // 2. Define float16_t -> float, uint8_t -> uint
    std::string patched = glsl;
    auto replace_all = [](std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    };

    replace_all(patched, "#error No extension available for FP16.", "// FP16 mapped to float");
    replace_all(patched, "#error No extension available for Int8.", "// Int8 mapped to uint");
    replace_all(patched, "#error No extension available for Int16.", "// Int16 mapped to int");

    std::string header = 
        "#version 430\n"
        "#define float16_t float\n"
        "#define f16vec2 vec2\n"
        "#define f16vec3 vec3\n"
        "#define f16vec4 vec4\n"
        "#define uint8_t uint\n"
        "#define u8vec2 uvec2\n"
        "#define u8vec3 uvec3\n"
        "#define u8vec4 uvec4\n"
        "#define int8_t int\n"
        "#define i8vec2 ivec2\n"
        "#define i8vec3 ivec3\n"
        "#define i8vec4 ivec4\n"
        "#define uint16_t uint\n"
        "#define int16_t int\n";

    if (patched.find("#version 430") == 0) {
        patched = header + patched.substr(12);
    }

    GLuint s = gl.CreateShader(GL_COMPUTE_SHADER);
    const char* cstr = patched.c_str();
    gl.ShaderSource(s, 1, &cstr, nullptr);
    gl.CompileShader(s);
    GLint status = 0;
    gl.GetShaderiv(s, GL_COMPILE_STATUS, &status);
    if (status) {
        std::cout << "SUCCESS: Patched shader compiled flawlessly on OpenGL 4.30!" << std::endl;
    } else {
        char log[1024];
        GLsizei len = 0;
        gl.GetShaderInfoLog(s, sizeof(log), &len, log);
        std::cout << "FAILED: " << log << std::endl;
    }
    gl.DeleteShader(s);

    return 0;
}
