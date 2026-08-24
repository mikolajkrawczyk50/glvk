# GLVK: Minimal GPGPU Vulkan Translation Layer over OpenGL 4.3+

**GLVK** is a lightweight, compute-focused Vulkan translation layer that maps Vulkan compute shaders, storage buffers (SSBOs), push constants, and dispatch commands directly to **OpenGL 4.3+ / OpenGL ES 3.1+**.

It is specifically tailored for deep learning inference runtimes like **ncnn-vulkan**, enabling Vulkan neural network acceleration on legacy GPUs that only have OpenGL 4.3+ drivers.

---

## Key Features

- **Compute & SSBO Centric**: Focuses strictly on Vulkan GPGPU features (Compute Pipelines, Storage Buffers, Descriptors, Push Constants, Specialization Constants, Memory Barriers, Fences).
- **SPIR-V to GLSL 4.30 on-the-fly**: Embedded [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) dynamically translates SPIR-V bytecode and specializes constants at pipeline creation.
- **Headless Context Support**: Supports headless execution via EGL (`EGL_PLATFORM_DEVICE_EXT` and surfaceless contexts).
- **Drop-in `libvulkan.so` Replacement**: Produces both `libglvk_static.a` and `libvulkan.so`.

---

## Project Structure

```
glvk/
├── CMakeLists.txt              # Root build configuration with SPIRV-Cross FetchContent
├── src/
│   ├── gl_loader.hpp           # OpenGL 4.3 core function pointers and definitions
│   ├── gl_backend.hpp / .cpp   # Headless EGL context and device capabilities query
│   ├── glvk_internal.hpp       # Vulkan handle structures (Device, Buffer, Pipeline, etc.)
│   ├── glvk_instance.cpp       # Instance, Physical Device, and Feature queries
│   ├── glvk_device.cpp         # Logical Device and Queue creation
│   ├── glvk_memory.cpp         # VkDeviceMemory allocation and host mapping (glMapBufferRange)
│   ├── glvk_buffer.cpp         # VkBuffer and memory requirement queries
│   ├── glvk_shader.hpp / .cpp  # SPIRV-Cross shader translation and specialization constants
│   ├── glvk_pipeline.cpp       # Pipeline layout and compute pipeline compilation
│   ├── glvk_descriptor.cpp     # Descriptor set layout, descriptor pools, and SSBO binding
│   ├── glvk_command.cpp        # Command buffer recording, push constants, and dispatch replay
│   ├── glvk_sync.cpp           # Fences (glFenceSync / glClientWaitSync) and semaphores
│   └── glvk_entrypoints.cpp    # vkGetInstanceProcAddr and vkGetDeviceProcAddr dispatch table
└── tests/
    ├── test_vector_add.cpp     # Vector addition test with push constants and SSBOs
    └── test_spec_constant.cpp  # Specialization constant validation test
```

---

## Building and Testing

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
ctest --output-on-failure
```

---

## Integrating with `ncnn-vulkan`

### Option 1: LD_LIBRARY_PATH (Dynamic Loading)
Compile `ncnn` with `-DNCNN_VULKAN=ON`. Run your ncnn application with:
```bash
LD_LIBRARY_PATH=/path/to/glvk/build:$LD_LIBRARY_PATH ./your_ncnn_app
```

### Option 2: Direct Static Linking
Link `glvk_static` directly into your application or ncnn build:
```cmake
target_link_libraries(your_target PRIVATE glvk_static)
```
