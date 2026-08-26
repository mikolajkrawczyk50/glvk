#pragma once

#include "gl_loader.hpp"
#include <string>
#include <vector>
#include <mutex>

struct GLGPUInfo {
    std::string device_name;
    std::string vendor_name;
    uint32_t vendor_id = 0x10DE; // default NVIDIA
    uint32_t device_id = 0x0DE0; // default Fermi GT 730
    uint32_t subgroup_size = 32; // Fermi warp size 32
    uint32_t max_compute_workgroup_invocations = 1024;
    uint32_t max_compute_workgroup_count[3] = { 65535, 65535, 65535 };
    uint32_t max_compute_workgroup_size[3] = { 1024, 1024, 64 };
    uint32_t max_compute_shared_memory_size = 32768;
    uint32_t min_ssbo_offset_alignment = 256;
    uint32_t max_ssbo_bindings = 16;
    uint64_t max_ssbo_size = 128 * 1024 * 1024;
    uint64_t total_memory = 1024 * 1024 * 1024; // 1 GB default estimation
};

class GLBackend {
public:
    static GLBackend& Instance();

    bool Initialize();
    void Shutdown();

    bool MakeCurrent();
    void DoneCurrent();

    const GLGPUInfo& GetGPUInfo() const { return gpu_info_; }
    bool IsInitialized() const { return initialized_; }

    std::recursive_mutex& GetMutex() { return mutex_; }

private:
    GLBackend() = default;
    ~GLBackend() { Shutdown(); }

    bool InitEGL();
    void QueryCapabilities();

    EGLDisplay egl_display_ = EGL_NO_DISPLAY;
    EGLContext egl_context_ = EGL_NO_CONTEXT;
    EGLSurface egl_surface_ = EGL_NO_SURFACE;
    bool initialized_ = false;
    int ref_count_ = 0;
    GLGPUInfo gpu_info_;
    std::recursive_mutex mutex_;
};

class GLVKContextScope {
public:
    GLVKContextScope() {
        GLBackend::Instance().GetMutex().lock();
        GLBackend::Instance().MakeCurrent();
    }

    ~GLVKContextScope() {
        GLBackend::Instance().DoneCurrent();
        GLBackend::Instance().GetMutex().unlock();
    }

    GLVKContextScope(const GLVKContextScope&) = delete;
    GLVKContextScope& operator=(const GLVKContextScope&) = delete;
};
