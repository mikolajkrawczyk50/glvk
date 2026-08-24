# GLVK - Minimal Vulkan GPGPU Compute Shim on OpenGL 4.3+

**GLVK** is a lightweight, drop-in Vulkan translation layer designed for **GPGPU compute workloads** (such as [ncnn-vulkan](https://github.com/Tencent/ncnn)). It translates Vulkan Compute APIs into **OpenGL 4.3+ Core Profile / OpenGL ES 3.1+ Compute Shaders**, allowing modern Vulkan compute software to run on legacy GPUs, APUs, and embedded platforms lacking native Vulkan drivers.

---

## ⚡ Key Features

- **Direct OpenGL 4.3+ SSBO & Compute Mapping**:
  - `VkBuffer` $\rightarrow$ OpenGL Shader Storage Buffer Objects (`GL_SHADER_STORAGE_BUFFER`)
  - `vkCmdDispatch` / `vkCmdDispatchIndirect` $\rightarrow$ `glDispatchCompute` / `glDispatchComputeIndirect`
  - `vkCmdPipelineBarrier` $\rightarrow$ `glMemoryBarrier`
  - `vkQueueSubmit` / `vkWaitForFences` $\rightarrow$ `glFenceSync` / `glClientWaitSync`
- **JIT SPIR-V to GLSL 4.30 Decompilation**:
  - Embedded **SPIRV-Cross** runtime converts Vulkan SPIR-V bytecode into optimized GLSL compute shaders.
  - Full support for `VkSpecializationInfo` specialization constants patched at runtime before GLSL compilation.
  - Automatic reflection & mapping of Vulkan Push Constants ($128$ bytes) to uniform variables (`glProgramUniform1i`, `glProgramUniform1f`, etc.).
- **Zero-Copy Host Visible Persistent Memory**:
  - Utilizes `glBufferStorage` with `GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT` for optimal CPU $\leftrightarrow$ GPU data staging.
- **Full ncnn-vulkan Compatibility**:
  - Provides the complete Vulkan runtime surface expected by `ncnn::create_gpu_instance()`, `ncnn::VulkanDevice`, and `ncnn::VkCompute`.

---

## 🛠️ Architecture

```
┌────────────────────────────────────────────────────────┐
│               ncnn-vulkan / GPGPU Apps                 │
└──────────────────────────┬─────────────────────────────┘
                           │ Vulkan C API (libvulkan.so)
┌──────────────────────────▼─────────────────────────────┐
│                          GLVK                          │
│ ┌────────────────────────────────────────────────────┐ │
│ │  SPIRV-Cross JIT Compiler + Spec Constant Patching  │ │
│ └────────────────────────────────────────────────────┘ │
│ ┌────────────────────────────────────────────────────┐ │
│ │  SSBO Memory Manager + Persistent Buffer Mapping   │ │
│ └────────────────────────────────────────────────────┘ │
│ ┌────────────────────────────────────────────────────┐ │
│ │  Command Recorder & Dispatch Engine                │ │
│ └────────────────────────────────────────────────────┘ │
└──────────────────────────┬─────────────────────────────┘
                           │ EGL / OpenGL 4.3+ Core
┌──────────────────────────▼─────────────────────────────┐
│                 GPU Driver (radeonsi / iris / etc.)     │
└────────────────────────────────────────────────────────┘
```

---

## 📦 What is Needed to Run `ncnn-vulkan`

To run `ncnn-vulkan` on top of GLVK:

### 1. Hardware & System Requirements
- **OpenGL 4.3+ Core Profile** or **OpenGL ES 3.1+** (Support for SSBOs, Compute Shaders, Image load/store).
- Linux with EGL support (`libEGL.so.1`, `libGL.so.1`).

### 2. Vulkan API Subset Required by ncnn
1. **Physical Device & Properties**:
   - `vkEnumeratePhysicalDevices`, `vkGetPhysicalDeviceProperties`, `vkGetPhysicalDeviceProperties2KHR`
   - Specialization and subgroup properties (`subgroupSize`, compute queue families, SSBO alignments).
2. **Buffer & Memory Allocation**:
   - `vkAllocateMemory`, `vkMapMemory`, `vkBindBufferMemory`
   - Host-visible and coherent storage buffers (SSBOs).
3. **Shader Pipelines & Dispatch**:
   - SPIR-V module creation (`vkCreateShaderModule`)
   - Specialization constant patching (`VkSpecializationInfo`)
   - Pipeline layout and descriptor sets (`VkDescriptorSetLayout`, `VkDescriptorPool`, `vkUpdateDescriptorSets`)
   - 128-byte push constants (`vkCmdPushConstants`)
   - Compute dispatch (`vkCmdDispatch`)
4. **Synchronization**:
   - `vkCmdPipelineBarrier`, `vkQueueSubmit`, `vkWaitForFences`

---

## 🚀 Building and Running

### Build GLVK
```bash
git clone /home/user/repos/glvk
cd glvk
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

### Run Tests
```bash
ctest --output-on-failure
```

Output:
```
1/3 Test #1: test_vector_add ..................   Passed    0.04 sec
2/3 Test #2: test_spec_constant ...............   Passed    0.04 sec
3/3 Test #3: test_ncnn_vulkan_layer ...........   Passed    0.17 sec

100% tests passed out of 3
```

---

## 💡 Running Any ncnn-vulkan Application with GLVK

You can redirect any existing binary dynamically to use GLVK:

```bash
LD_LIBRARY_PATH=/home/user/repos/glvk/build:$LD_LIBRARY_PATH ./your_ncnn_app
```
