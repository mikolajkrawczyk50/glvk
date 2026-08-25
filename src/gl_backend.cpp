#include "gl_backend.hpp"
#include <iostream>
#include <vector>
#include <cstring>

GLFunctions gl;

bool LoadGLFunctions() {
    #define LOAD_PROC(type, name) \
        gl.name = (type)eglGetProcAddress("gl" #name); \
        if (!gl.name) { \
            std::cerr << "[GLVK] Failed to load gl" #name << std::endl; \
        }

    LOAD_PROC(PFNGLGETERRORPROC, GetError);
    LOAD_PROC(PFNGLDISPATCHCOMPUTEPROC, DispatchCompute);
    LOAD_PROC(PFNGLDISPATCHCOMPUTEINDIRECTPROC, DispatchComputeIndirect);
    LOAD_PROC(PFNGLBINDBUFFERBASEPROC, BindBufferBase);
    LOAD_PROC(PFNGLBINDBUFFERRANGEPROC, BindBufferRange);
    LOAD_PROC(PFNGLBUFFERSTORAGEPROC, BufferStorage);
    LOAD_PROC(PFNGLMEMORYBARRIERPROC, MemoryBarrier);

    LOAD_PROC(PFNGLGENBUFFERSPROC, GenBuffers);
    LOAD_PROC(PFNGLDELETEBUFFERSPROC, DeleteBuffers);
    LOAD_PROC(PFNGLBINDBUFFERPROC, BindBuffer);
    LOAD_PROC(PFNGLBUFFERDATAPROC, BufferData);
    LOAD_PROC(PFNGLBUFFERSUBDATAPROC, BufferSubData);
    LOAD_PROC(PFNGLGETBUFFERSUBDATAPROC, GetBufferSubData);
    LOAD_PROC(PFNGLCOPYBUFFERSUBDATAPROC, CopyBufferSubData);
    LOAD_PROC(PFNGLMAPBUFFERRANGEPROC, MapBufferRange);
    LOAD_PROC(PFNGLUNMAPBUFFERPROC, UnmapBuffer);
    LOAD_PROC(PFNGLFLUSHMAPPEDBUFFERRANGEPROC, FlushMappedBufferRange);

    LOAD_PROC(PFNGLCREATESHADERPROC, CreateShader);
    LOAD_PROC(PFNGLSHADERSOURCEPROC, ShaderSource);
    LOAD_PROC(PFNGLCOMPILESHADERPROC, CompileShader);
    LOAD_PROC(PFNGLGETSHADERIVPROC, GetShaderiv);
    LOAD_PROC(PFNGLGETSHADERINFOLOGPROC, GetShaderInfoLog);
    LOAD_PROC(PFNGLDELETESHADERPROC, DeleteShader);

    LOAD_PROC(PFNGLCREATEPROGRAMPROC, CreateProgram);
    LOAD_PROC(PFNGLATTACHSHADERPROC, AttachShader);
    LOAD_PROC(PFNGLDETACHSHADERPROC, DetachShader);
    LOAD_PROC(PFNGLLINKPROGRAMPROC, LinkProgram);
    LOAD_PROC(PFNGLGETPROGRAMIVPROC, GetProgramiv);
    LOAD_PROC(PFNGLGETPROGRAMINFOLOGPROC, GetProgramInfoLog);
    LOAD_PROC(PFNGLUSEPROGRAMPROC, UseProgram);
    LOAD_PROC(PFNGLDELETEPROGRAMPROC, DeleteProgram);

    LOAD_PROC(PFNGLFENCESYNCPROC, FenceSync);
    LOAD_PROC(PFNGLISSYNCPROC, IsSync);
    LOAD_PROC(PFNGLDELETESYNCPROC, DeleteSync);
    LOAD_PROC(PFNGLCLIENTWAITSYNCPROC, ClientWaitSync);
    LOAD_PROC(PFNGLWAITSYNCPROC, WaitSync);

    LOAD_PROC(PFNGLGETUNIFORMLOCATIONPROC, GetUniformLocation);
    LOAD_PROC(PFNGLPROGRAMUNIFORM1IPROC, ProgramUniform1i);
    LOAD_PROC(PFNGLPROGRAMUNIFORM1UIPROC, ProgramUniform1ui);
    LOAD_PROC(PFNGLPROGRAMUNIFORM1FPROC, ProgramUniform1f);
    LOAD_PROC(PFNGLPROGRAMUNIFORM2FVPROC, ProgramUniform2fv);
    LOAD_PROC(PFNGLPROGRAMUNIFORM3FVPROC, ProgramUniform3fv);
    LOAD_PROC(PFNGLPROGRAMUNIFORM4FVPROC, ProgramUniform4fv);
    LOAD_PROC(PFNGLPROGRAMUNIFORM2IVPROC, ProgramUniform2iv);
    LOAD_PROC(PFNGLPROGRAMUNIFORM3IVPROC, ProgramUniform3iv);
    LOAD_PROC(PFNGLPROGRAMUNIFORM4IVPROC, ProgramUniform4iv);
    LOAD_PROC(PFNGLPROGRAMUNIFORM2UIVPROC, ProgramUniform2uiv);
    LOAD_PROC(PFNGLPROGRAMUNIFORM3UIVPROC, ProgramUniform3uiv);
    LOAD_PROC(PFNGLPROGRAMUNIFORM4UIVPROC, ProgramUniform4uiv);

    LOAD_PROC(PFNGLGETINTEGERI_VPROC, GetIntegeri_v);
    LOAD_PROC(PFNGLGETINTEGER64VPROC, GetInteger64v);

    #undef LOAD_PROC
    return true;
}

GLBackend& GLBackend::Instance() {
    static GLBackend instance;
    return instance;
}

bool GLBackend::Initialize() {
    if (initialized_) return true;

    // Prevent Mesa Gallium asynchronous worker thread (gdrv0) aborts on Intel and legacy GPUs
    setenv("GALLIUM_THREAD", "0", 0);
    setenv("mesa_glthread", "false", 0);

    if (!InitEGL()) {
        return false;
    }

    if (!LoadGLFunctions()) {
        return false;
    }

    QueryCapabilities();
    DoneCurrent();
    initialized_ = true;
    return true;
}

bool GLBackend::InitEGL() {
    PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT = 
        (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = 
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");

    EGLint major, minor;
    if (eglQueryDevicesEXT && eglGetPlatformDisplayEXT) {
        EGLint num_devices = 0;
        eglQueryDevicesEXT(0, nullptr, &num_devices);
        if (num_devices > 0) {
            std::vector<EGLDeviceEXT> devices(num_devices);
            eglQueryDevicesEXT(num_devices, devices.data(), &num_devices);
            for (int i = 0; i < num_devices; i++) {
                EGLDisplay dpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[i], nullptr);
                if (dpy != EGL_NO_DISPLAY && eglInitialize(dpy, &major, &minor)) {
                    egl_display_ = dpy;
                    break;
                }
            }
        }
    }

    if (egl_display_ == EGL_NO_DISPLAY) {
        egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (egl_display_ == EGL_NO_DISPLAY || !eglInitialize(egl_display_, &major, &minor)) {
            std::cerr << "[GLVK] Failed to get/initialize EGL display." << std::endl;
            return false;
        }
    }

    eglBindAPI(EGL_OPENGL_API);

    const EGLint context_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };

    egl_context_ = eglCreateContext(egl_display_, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, context_attribs);
    if (egl_context_ == EGL_NO_CONTEXT) {
        // Fallback: try choosing an EGLConfig for desktop OpenGL (needed on legacy Fermi drivers)
        const EGLint config_attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_NONE
        };
        EGLConfig config = nullptr;
        EGLint num_configs = 0;
        if (eglChooseConfig(egl_display_, config_attribs, &config, 1, &num_configs) && num_configs > 0) {
            egl_context_ = eglCreateContext(egl_display_, config, EGL_NO_CONTEXT, context_attribs);
            if (egl_context_ != EGL_NO_CONTEXT) {
                const EGLint pbuf_attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
                egl_surface_ = eglCreatePbufferSurface(egl_display_, config, pbuf_attribs);
            }
        }
    }

    if (egl_context_ == EGL_NO_CONTEXT) {
        // Try OpenGL ES 3.1 compute fallback
        eglBindAPI(EGL_OPENGL_ES_API);
        const EGLint es_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_CONTEXT_MINOR_VERSION, 1,
            EGL_NONE
        };
        egl_context_ = eglCreateContext(egl_display_, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, es_attribs);
    }

    if (egl_context_ == EGL_NO_CONTEXT) {
        std::cerr << "[GLVK] Failed to create EGL context (OpenGL 4.3+ Core or GLES 3.1+ required)." << std::endl;
        return false;
    }

    EGLSurface surf = (egl_surface_ != EGL_NO_SURFACE) ? egl_surface_ : EGL_NO_SURFACE;
    if (!eglMakeCurrent(egl_display_, surf, surf, egl_context_)) {
        std::cerr << "[GLVK] Failed to make EGL context current." << std::endl;
        return false;
    }

    return true;
}

void GLBackend::Shutdown() {
    if (!initialized_) return;
    if (egl_display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (egl_surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(egl_display_, egl_surface_);
            egl_surface_ = EGL_NO_SURFACE;
        }
        if (egl_context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(egl_display_, egl_context_);
            egl_context_ = EGL_NO_CONTEXT;
        }
        // Skip eglTerminate: on Nouveau DRI, eglTerminate blocks on kernel driver lock.
        egl_display_ = EGL_NO_DISPLAY;
    }
    initialized_ = false;
}

bool GLBackend::MakeCurrent() {
    if (egl_display_ == EGL_NO_DISPLAY || egl_context_ == EGL_NO_CONTEXT) return false;
    if (eglGetCurrentContext() == egl_context_) return true;
    EGLSurface surf = (egl_surface_ != EGL_NO_SURFACE) ? egl_surface_ : EGL_NO_SURFACE;
    return eglMakeCurrent(egl_display_, surf, surf, egl_context_) == EGL_TRUE;
}

void GLBackend::DoneCurrent() {
    if (egl_display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
}

void GLBackend::QueryCapabilities() {
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);

    gpu_info_.device_name = renderer ? reinterpret_cast<const char*>(renderer) : "OpenGL Compute Device";
    gpu_info_.vendor_name = vendor ? reinterpret_cast<const char*>(vendor) : "Unknown Vendor";

    std::string vstr = gpu_info_.vendor_name;
    std::string rstr = gpu_info_.device_name;

    // Detect NVIDIA / Fermi GF108 / Kepler / AMD / Intel
    if (vstr.find("NVIDIA") != std::string::npos || vstr.find("nvidia") != std::string::npos ||
        rstr.find("NVIDIA") != std::string::npos || rstr.find("GeForce") != std::string::npos ||
        vstr.find("nouveau") != std::string::npos || rstr.find("nouveau") != std::string::npos ||
        rstr.find("NVC") != std::string::npos) {
        gpu_info_.vendor_id = 0x10DE; // NVIDIA
        gpu_info_.device_id = 0x0DE0; // GT 730 / Fermi GF108 default
        gpu_info_.subgroup_size = 32; // NVIDIA warp size is 32
        if (rstr.find("730") != std::string::npos || rstr.find("GF108") != std::string::npos || rstr.find("NVC1") != std::string::npos) {
            gpu_info_.device_id = 0x0DE0;
            // Fermi hardware does not support mthd 0x27e0 (glMemoryBarrier).
            // Calling glMemoryBarrier triggers FIFO SCHED_ERROR 0d. Disable it on Fermi.
            gl.MemoryBarrier = nullptr;
        }
    } else if (vstr.find("AMD") != std::string::npos || vstr.find("ATI") != std::string::npos ||
               vstr.find("Radeon") != std::string::npos || rstr.find("Radeon") != std::string::npos) {
        gpu_info_.vendor_id = 0x1002; // AMD
        gpu_info_.device_id = 0x73BF;
        gpu_info_.subgroup_size = 64; // AMD wavefront size is 64
    } else if (vstr.find("Intel") != std::string::npos || rstr.find("Intel") != std::string::npos) {
        gpu_info_.vendor_id = 0x8086; // Intel
        gpu_info_.device_id = 0x9BC5;
        gpu_info_.subgroup_size = 32;
    } else {
        gpu_info_.vendor_id = 0x10DE;
        gpu_info_.device_id = 0x0DE0;
        gpu_info_.subgroup_size = 32;
    }

    if (gl.GetIntegeri_v) {
        gl.GetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, (GLint*)&gpu_info_.max_compute_workgroup_count[0]);
        gl.GetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, (GLint*)&gpu_info_.max_compute_workgroup_count[1]);
        gl.GetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, (GLint*)&gpu_info_.max_compute_workgroup_count[2]);

        gl.GetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, (GLint*)&gpu_info_.max_compute_workgroup_size[0]);
        gl.GetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, (GLint*)&gpu_info_.max_compute_workgroup_size[1]);
        gl.GetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, (GLint*)&gpu_info_.max_compute_workgroup_size[2]);
    }

    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, (GLint*)&gpu_info_.max_compute_workgroup_invocations);
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, (GLint*)&gpu_info_.max_compute_shared_memory_size);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, (GLint*)&gpu_info_.max_ssbo_bindings);

    GLint align = 256;
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &align);
    gpu_info_.min_ssbo_offset_alignment = align > 0 ? align : 256;

    if (gl.GetInteger64v) {
        GLint64 max_block_size = 0;
        gl.GetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_block_size);
        gpu_info_.max_ssbo_size = max_block_size > 0 ? max_block_size : (1ULL << 30);
    } else {
        gpu_info_.max_ssbo_size = 1ULL << 30;
    }

    gpu_info_.total_memory = 2ULL * 1024 * 1024 * 1024; // 2GB
}
