#include "glvk_internal.hpp"
#include <cstring>
#include <iostream>
#include <vector>
#include <mutex>

struct PooledBuffer {
    GLuint gl_buffer = 0;
    VkDeviceSize size = 0;
    void* mapped_ptr = nullptr;
    uint32_t memory_type_index = 0;
};

static std::mutex g_pool_mutex;
static std::vector<PooledBuffer> g_buffer_pool;
static size_t g_pool_total_bytes = 0;
static const size_t MAX_POOL_TOTAL_BYTES = 0; // Disable buffer pooling to avoid driver aliasing

// Round up allocation sizes to power-of-two (or 64KB multiple) to maximize pool reuse
static VkDeviceSize PoolBucketSize(VkDeviceSize size) {
    if (size < 65536) return 65536;
    VkDeviceSize v = size - 1;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    return v + 1;
}

static const VkDeviceSize MAX_BANK_SIZE = 128 * 1024 * 1024; // 128 MB per hardware bank

extern "C" {

VkResult vkAllocateMemory(VkDevice device,
                          const VkMemoryAllocateInfo* pAllocateInfo,
                          const VkAllocationCallbacks* pAllocator,
                          VkDeviceMemory* pMemory) {
    if (!device || !pAllocateInfo || !pMemory) return VK_ERROR_INITIALIZATION_FAILED;

    VkDeviceSize requested_size = pAllocateInfo->allocationSize;

    // Multi-bank allocation for buffers > 128 MB
    if (requested_size > MAX_BANK_SIZE) {
        GLVKContextScope scope;

        auto mem = new VkDeviceMemory_T();
        mem->size = requested_size;
        mem->memory_type_index = pAllocateInfo->memoryTypeIndex;
        mem->shadow_ptr = malloc(requested_size);
        if (mem->shadow_ptr) {
            memset(mem->shadow_ptr, 0, requested_size);
        }
        mem->mapped_ptr = mem->shadow_ptr;
        mem->mapped_offset = 0;
        mem->mapped_size = requested_size;

        VkDeviceSize remaining = requested_size;
        while (remaining > 0) {
            VkDeviceSize bsize = std::min(MAX_BANK_SIZE, remaining);
            GLuint buf = 0;
            gl.GenBuffers(1, &buf);
            gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
            if (gl.BufferStorage) {
                GLbitfield flags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
                gl.BufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)bsize, nullptr, flags);
            } else {
                gl.BufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)bsize, nullptr, GL_DYNAMIC_DRAW);
            }
            gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            mem->gl_buffers.push_back(buf);
            mem->bank_sizes.push_back(bsize);
            remaining -= bsize;
        }

        mem->gl_buffer = mem->gl_buffers.empty() ? 0 : mem->gl_buffers[0];
        *pMemory = mem;
        return VK_SUCCESS;
    }

    VkDeviceSize alloc_size = PoolBucketSize(requested_size);

    // 1. Try reusing a cached buffer from the pool
    {
        std::lock_guard<std::mutex> lock(g_pool_mutex);
        for (auto it = g_buffer_pool.begin(); it != g_buffer_pool.end(); ++it) {
            if (it->size >= requested_size && it->size <= alloc_size * 2 &&
                it->memory_type_index == pAllocateInfo->memoryTypeIndex) {
                auto mem = new VkDeviceMemory_T();
                mem->size = it->size;
                mem->memory_type_index = it->memory_type_index;
                mem->gl_buffer = it->gl_buffer;
                mem->gl_buffers = { it->gl_buffer };
                mem->bank_sizes = { it->size };
                mem->mapped_ptr = it->mapped_ptr;
                mem->mapped_offset = 0;
                mem->mapped_size = it->size;
                if (g_pool_total_bytes >= it->size) {
                    g_pool_total_bytes -= it->size;
                } else {
                    g_pool_total_bytes = 0;
                }
                g_buffer_pool.erase(it);
                *pMemory = mem;
                return VK_SUCCESS;
            }
        }
    }

    // 2. Allocate single OpenGL buffer object
    GLVKContextScope scope;

    auto mem = new VkDeviceMemory_T();
    mem->size = alloc_size;
    mem->memory_type_index = pAllocateInfo->memoryTypeIndex;

    gl.GenBuffers(1, &mem->gl_buffer);
    gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, mem->gl_buffer);
    gl.BufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)mem->size, nullptr, GL_DYNAMIC_DRAW);
    gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    mem->gl_buffers = { mem->gl_buffer };
    mem->bank_sizes = { mem->size };

    *pMemory = mem;
    return VK_SUCCESS;
}

void vkFreeMemory(VkDevice device,
                  VkDeviceMemory memory,
                  const VkAllocationCallbacks* pAllocator) {
    if (!memory) return;

    if (memory->shadow_ptr) {
        free(memory->shadow_ptr);
        memory->shadow_ptr = nullptr;
    }

    if (memory->gl_buffers.size() > 1) {
        GLVKContextScope scope;
        for (GLuint b : memory->gl_buffers) {
            if (b) gl.DeleteBuffers(1, &b);
        }
        delete memory;
        return;
    }

    // Cache single buffer in pool for instant reuse without kernel GEM churn
    {
        std::lock_guard<std::mutex> lock(g_pool_mutex);
        if (g_buffer_pool.size() < 64 && (g_pool_total_bytes + memory->size) <= MAX_POOL_TOTAL_BYTES) {
            PooledBuffer pb;
            pb.gl_buffer = memory->gl_buffer;
            pb.size = memory->size;
            pb.mapped_ptr = memory->mapped_ptr;
            pb.memory_type_index = memory->memory_type_index;
            g_buffer_pool.push_back(pb);
            g_pool_total_bytes += memory->size;
            delete memory;
            return;
        }
    }

    // Pool full: destroy buffer safely after GPU completion
    GLVKContextScope scope;

    if (memory->mapped_ptr) {
        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, memory->gl_buffer);
        gl.UnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        memory->mapped_ptr = nullptr;
    }
    if (memory->gl_buffer) {
        gl.DeleteBuffers(1, &memory->gl_buffer);
        memory->gl_buffer = 0;
    }
    delete memory;
}

VkResult vkMapMemory(VkDevice device,
                     VkDeviceMemory memory,
                     VkDeviceSize offset,
                     VkDeviceSize size,
                     VkMemoryMapFlags flags,
                     void** ppData) {
    if (!memory || !ppData) return VK_ERROR_MEMORY_MAP_FAILED;

    if (!memory->shadow_ptr) {
        memory->shadow_ptr = malloc((size_t)memory->size);
        if (!memory->shadow_ptr) {
            *ppData = nullptr;
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        memset(memory->shadow_ptr, 0, (size_t)memory->size);
        if (memory->gl_buffer) {
            GLVKContextScope scope;
            gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, memory->gl_buffer);
            gl.GetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)memory->size, memory->shadow_ptr);
            gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }

    *ppData = (uint8_t*)memory->shadow_ptr + offset;
    return VK_SUCCESS;
}

void vkUnmapMemory(VkDevice device, VkDeviceMemory memory) {
    if (!memory || !memory->shadow_ptr) return;

    GLVKContextScope scope;
    if (memory->gl_buffers.size() > 1) {
        for (size_t b = 0; b < memory->gl_buffers.size(); b++) {
            gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, memory->gl_buffers[b]);
            gl.BufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)memory->bank_sizes[b],
                             (const uint8_t*)memory->shadow_ptr + b * MAX_BANK_SIZE);
            gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    } else if (memory->gl_buffer) {
        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, memory->gl_buffer);
        gl.BufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)memory->size, memory->shadow_ptr);
        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

VkResult vkFlushMappedMemoryRanges(VkDevice device,
                                   uint32_t memoryRangeCount,
                                   const VkMappedMemoryRange* pMemoryRanges) {
    if (!pMemoryRanges) return VK_SUCCESS;
    GLVKContextScope scope;
    for (uint32_t i = 0; i < memoryRangeCount; i++) {
        auto mem = pMemoryRanges[i].memory;
        if (mem && mem->shadow_ptr && mem->gl_buffer) {
            gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, mem->gl_buffer);
            gl.BufferSubData(GL_SHADER_STORAGE_BUFFER, (GLintptr)pMemoryRanges[i].offset,
                             (GLsizeiptr)pMemoryRanges[i].size,
                             (const uint8_t*)mem->shadow_ptr + pMemoryRanges[i].offset);
            gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }
    return VK_SUCCESS;
}

VkResult vkInvalidateMappedMemoryRanges(VkDevice device,
                                        uint32_t memoryRangeCount,
                                        const VkMappedMemoryRange* pMemoryRanges) {
    GLVKContextScope scope;
    for (uint32_t i = 0; i < memoryRangeCount; i++) {
        auto mem = pMemoryRanges[i].memory;
        if (mem && mem->shadow_ptr && mem->gl_buffers.size() > 1) {
            for (size_t b = 0; b < mem->gl_buffers.size(); b++) {
                gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, mem->gl_buffers[b]);
                gl.GetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)mem->bank_sizes[b],
                                    (uint8_t*)mem->shadow_ptr + b * MAX_BANK_SIZE);
                gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            }
        }
    }
    if (gl.MemoryBarrier) {
        gl.MemoryBarrier(GL_ALL_BARRIER_BITS);
    }
    return VK_SUCCESS;
}

VkResult vkBindBufferMemory(VkDevice device,
                            VkBuffer buffer,
                            VkDeviceMemory memory,
                            VkDeviceSize memoryOffset) {
    if (!buffer || !memory) return VK_ERROR_INITIALIZATION_FAILED;

    buffer->memory = memory;
    buffer->memory_offset = memoryOffset;
    return VK_SUCCESS;
}

VkResult vkBindBufferMemory2(VkDevice device,
                             uint32_t bindInfoCount,
                             const VkBindBufferMemoryInfo* pBindInfos) {
    for (uint32_t i = 0; i < bindInfoCount; i++) {
        const auto& info = pBindInfos[i];
        VkResult res = vkBindBufferMemory(device, info.buffer, info.memory, info.memoryOffset);
        if (res != VK_SUCCESS) return res;
    }
    return VK_SUCCESS;
}

VkResult vkBindImageMemory(VkDevice device,
                           VkImage image,
                           VkDeviceMemory memory,
                           VkDeviceSize memoryOffset) {
    return VK_SUCCESS;
}

VkResult vkBindImageMemory2(VkDevice device,
                            uint32_t bindInfoCount,
                            const VkBindImageMemoryInfo* pBindInfos) {
    return VK_SUCCESS;
}

void vkGetDeviceMemoryCommitment(VkDevice device,
                                 VkDeviceMemory memory,
                                 VkDeviceSize* pCommittedMemoryInBytes) {
    if (pCommittedMemoryInBytes && memory) {
        *pCommittedMemoryInBytes = memory->size;
    }
}

} // extern "C"
