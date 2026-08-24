#include "gl_backend.hpp"
#include <iostream>
#include <dlfcn.h>
#include <cstring>

GLFunctions gl;

static void* GetProc(const char* name, void* gl_lib) {
    void* p = (void*)eglGetProcAddress(name);
    if (!p && gl_lib) {
        p = dlsym(gl_lib, name);
    }
    return p;
}

bool LoadGLFunctions() {
    void* gl_lib = dlopen("libGL.so.1", RTLD_LAZY | RTLD_GLOBAL);
    if (!gl_lib) {
        gl_lib = dlopen("libOpenGL.so.0", RTLD_LAZY | RTLD_GLOBAL);
    }

    gl.DispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)GetProc("glDispatchCompute", gl_lib);
    gl.DispatchComputeIndirect = (PFNGLDISPATCHCOMPUTEINDIRECTPROC)GetProc("glDispatchComputeIndirect", gl_lib);
    gl.BindBufferBase = (PFNGLBINDBUFFERBASEPROC)GetProc("glBindBufferBase", gl_lib);
    gl.BindBufferRange = (PFNGLBINDBUFFERRANGEPROC)GetProc("glBindBufferRange", gl_lib);
    gl.BufferStorage = (PFNGLBUFFERSTORAGEPROC)GetProc("glBufferStorage", gl_lib);
    gl.MemoryBarrier = (PFNGLMEMORYBARRIERPROC)GetProc("glMemoryBarrier", gl_lib);
    gl.GenBuffers = (PFNGLGENBUFFERSPROC)GetProc("glGenBuffers", gl_lib);
    gl.DeleteBuffers = (PFNGLDELETEBUFFERSPROC)GetProc("glDeleteBuffers", gl_lib);
    gl.BindBuffer = (PFNGLBINDBUFFERPROC)GetProc("glBindBuffer", gl_lib);
    gl.BufferData = (PFNGLBUFFERDATAPROC)GetProc("glBufferData", gl_lib);
    gl.BufferSubData = (PFNGLBUFFERSUBDATAPROC)GetProc("glBufferSubData", gl_lib);
    gl.CopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)GetProc("glCopyBufferSubData", gl_lib);
    gl.MapBufferRange = (PFNGLMAPBUFFERRANGEPROC)GetProc("glMapBufferRange", gl_lib);
    gl.UnmapBuffer = (PFNGLUNMAPBUFFERPROC)GetProc("glUnmapBuffer", gl_lib);
    gl.FlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)GetProc("glFlushMappedBufferRange", gl_lib);

    gl.CreateShader = (PFNGLCREATESHADERPROC)GetProc("glCreateShader", gl_lib);
    gl.ShaderSource = (PFNGLSHADERSOURCEPROC)GetProc("glShaderSource", gl_lib);
    gl.CompileShader = (PFNGLCOMPILESHADERPROC)GetProc("glCompileShader", gl_lib);
    gl.GetShaderiv = (PFNGLGETSHADERIVPROC)GetProc("glGetShaderiv", gl_lib);
    gl.GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)GetProc("glGetShaderInfoLog", gl_lib);
    gl.DeleteShader = (PFNGLDELETESHADERPROC)GetProc("glDeleteShader", gl_lib);

    gl.CreateProgram = (PFNGLCREATEPROGRAMPROC)GetProc("glCreateProgram", gl_lib);
    gl.AttachShader = (PFNGLATTACHSHADERPROC)GetProc("glAttachShader", gl_lib);
    gl.DetachShader = (PFNGLDETACHSHADERPROC)GetProc("glDetachShader", gl_lib);
    gl.LinkProgram = (PFNGLLINKPROGRAMPROC)GetProc("glLinkProgram", gl_lib);
    gl.GetProgramiv = (PFNGLGETPROGRAMIVPROC)GetProc("glGetProgramiv", gl_lib);
    gl.GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)GetProc("glGetProgramInfoLog", gl_lib);
    gl.UseProgram = (PFNGLUSEPROGRAMPROC)GetProc("glUseProgram", gl_lib);
    gl.DeleteProgram = (PFNGLDELETEPROGRAMPROC)GetProc("glDeleteProgram", gl_lib);

    gl.FenceSync = (PFNGLFENCESYNCPROC)GetProc("glFenceSync", gl_lib);
    gl.IsSync = (PFNGLISSYNCPROC)GetProc("glIsSync", gl_lib);
    gl.DeleteSync = (PFNGLDELETESYNCPROC)GetProc("glDeleteSync", gl_lib);
    gl.ClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)GetProc("glClientWaitSync", gl_lib);
    gl.WaitSync = (PFNGLWAITSYNCPROC)GetProc("glWaitSync", gl_lib);

    gl.GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GetProc("glGetUniformLocation", gl_lib);
    gl.ProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)GetProc("glProgramUniform1i", gl_lib);
    gl.ProgramUniform4fv = (PFNGLPROGRAMUNIFORM4FVPROC)GetProc("glProgramUniform4fv", gl_lib);
    gl.ProgramUniform4iv = (PFNGLPROGRAMUNIFORM4IVPROC)GetProc("glProgramUniform4iv", gl_lib);
    gl.ProgramUniform4uiv = (PFNGLPROGRAMUNIFORM4UIVPROC)GetProc("glProgramUniform4uiv", gl_lib);

    gl.GetIntegeri_v = (PFNGLGETINTEGERI_VPROC)GetProc("glGetIntegeri_v", gl_lib);
    gl.GetInteger64v = (PFNGLGETINTEGER64VPROC)GetProc("glGetInteger64v", gl_lib);

    return (gl.DispatchCompute && gl.BindBufferBase && gl.CreateShader && gl.CreateProgram);
}

GLBackend& GLBackend::Instance() {
    static GLBackend instance;
    return instance;
}

bool GLBackend::InitEGL() {
    PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT = 
        (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = 
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");

    egl_display_ = EGL_NO_DISPLAY;

    if (eglQueryDevicesEXT && eglGetPlatformDisplayEXT) {
        EGLDeviceEXT devices[16];
        EGLint num_devices = 0;
        if (eglQueryDevicesEXT(16, devices, &num_devices) && num_devices > 0) {
            egl_display_ = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[0], nullptr);
        }
    }

    if (egl_display_ == EGL_NO_DISPLAY) {
        egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }

    if (egl_display_ == EGL_NO_DISPLAY) {
        return false;
    }

    EGLint major = 0, minor = 0;
    if (!eglInitialize(egl_display_, &major, &minor)) {
        return false;
    }

    eglBindAPI(EGL_OPENGL_API);

    static const EGLint ctx_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };

    egl_context_ = eglCreateContext(egl_display_, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, ctx_attribs);
    if (egl_context_ == EGL_NO_CONTEXT) {
        // Try fallback to OpenGL ES 3.1
        eglBindAPI(EGL_OPENGL_ES_API);
        static const EGLint es_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_CONTEXT_MINOR_VERSION, 1,
            EGL_NONE
        };
        egl_context_ = eglCreateContext(egl_display_, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, es_attribs);
        if (egl_context_ == EGL_NO_CONTEXT) {
            return false;
        }
    }

    if (!eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context_)) {
        return false;
    }

    if (!LoadGLFunctions()) {
        return false;
    }

    return true;
}

void GLBackend::QueryCapabilities() {
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* vendor = (const char*)glGetString(GL_VENDOR);

    gpu_info_.device_name = renderer ? renderer : "OpenGL Compute Device";
    gpu_info_.vendor_name = vendor ? vendor : "Unknown";

    if (gpu_info_.vendor_name.find("NVIDIA") != std::string::npos) {
        gpu_info_.vendor_id = 0x10DE;
    } else if (gpu_info_.vendor_name.find("AMD") != std::string::npos ||
               gpu_info_.vendor_name.find("ATI") != std::string::npos ||
               gpu_info_.vendor_name.find("Advanced Micro Devices") != std::string::npos) {
        gpu_info_.vendor_id = 0x1002;
    } else if (gpu_info_.vendor_name.find("Intel") != std::string::npos) {
        gpu_info_.vendor_id = 0x8086;
    } else if (gpu_info_.vendor_name.find("ARM") != std::string::npos) {
        gpu_info_.vendor_id = 0x13B5;
    } else if (gpu_info_.vendor_name.find("Qualcomm") != std::string::npos) {
        gpu_info_.vendor_id = 0x5143;
    } else {
        gpu_info_.vendor_id = 0x10005; // Generic
    }
    gpu_info_.device_id = 0x0001;

    GLint max_invocations = 1024;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_invocations);
    gpu_info_.max_compute_workgroup_invocations = max_invocations;

    for (int i = 0; i < 3; i++) {
        GLint val = 0;
        if (gl.GetIntegeri_v) {
            gl.GetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, &val);
            gpu_info_.max_compute_workgroup_count[i] = val > 0 ? val : 65535;

            gl.GetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, &val);
            gpu_info_.max_compute_workgroup_size[i] = val > 0 ? val : 1024;
        } else {
            gpu_info_.max_compute_workgroup_count[i] = 65535;
            gpu_info_.max_compute_workgroup_size[i] = 1024;
        }
    }

    GLint shared_mem = 32768;
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &shared_mem);
    gpu_info_.max_compute_shared_memory_size = shared_mem;

    GLint ssbo_align = 256;
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &ssbo_align);
    gpu_info_.min_ssbo_offset_alignment = ssbo_align > 0 ? ssbo_align : 256;

    GLint max_ssbo = 16;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &max_ssbo);
    gpu_info_.max_ssbo_bindings = max_ssbo > 0 ? max_ssbo : 16;

    GLint64 max_ssbo_size = 128 * 1024 * 1024;
    if (gl.GetInteger64v) {
        gl.GetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_ssbo_size);
    }
    gpu_info_.max_ssbo_size = max_ssbo_size > 0 ? max_ssbo_size : (128 * 1024 * 1024);
}

bool GLBackend::Initialize() {
    if (initialized_) return true;

    if (!InitEGL()) {
        return false;
    }

    QueryCapabilities();
    initialized_ = true;
    return true;
}

bool GLBackend::MakeCurrent() {
    if (egl_display_ != EGL_NO_DISPLAY && egl_context_ != EGL_NO_CONTEXT) {
        return eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context_);
    }
    return false;
}

void GLBackend::DoneCurrent() {
    if (egl_display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
}

void GLBackend::Shutdown() {
    if (egl_display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (egl_context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(egl_display_, egl_context_);
            egl_context_ = EGL_NO_CONTEXT;
        }
        eglTerminate(egl_display_);
        egl_display_ = EGL_NO_DISPLAY;
    }
    initialized_ = false;
}
