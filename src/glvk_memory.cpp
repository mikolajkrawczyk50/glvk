#include "glvk_internal.hpp"
#include <cstring>
#include <iostream>

extern "C" {

VkResult vkAllocateMemory(VkDevice device,
                          const VkMemoryAllocateInfo* pAllocateInfo,
                          const VkAllocationCallbacks* pAllocator,
                          VkDeviceMemory* pMemory) {
    if (!device || !pAllocateInfo || !pMemory) return VK_ERROR_INITIALIZATION_FAILED;

    auto mem = new VkDeviceMemory_T();
    mem->size = pAllocateInfo->allocationSize;
    mem->memory_type_index = pAllocateInfo->memoryTypeIndex;

    gl.GenBuffers(1, &mem->gl_buffer);
    gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, mem->gl_buffer);

    if (gl.BufferStorage) {
        GLbitfield flags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        gl.BufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)mem->size, nullptr, flags);
    } else {
        gl.BufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)mem->size, nullptr, GL_DYNAMIC_DRAW);
    }

    gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    *pMemory = mem;
    return VK_SUCCESS;
}

void vkFreeMemory(VkDevice device,
                  VkDeviceMemory memory,
                  const VkAllocationCallbacks* pAllocator) {
    if (!memory) return;
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

    VkDeviceSize map_size = (size == VK_WHOLE_SIZE) ? (memory->size - offset) : size;

    if (memory->mapped_ptr) {
        *ppData = (uint8_t*)memory->mapped_ptr + offset;
        return VK_SUCCESS;
    }

    gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, memory->gl_buffer);

    GLbitfield access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    void* ptr = gl.MapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)memory->size, access);
    gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (!ptr) {
        // Fallback without persistent flags if not supported
        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, memory->gl_buffer);
        ptr = gl.MapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)memory->size, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    if (!ptr) {
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    memory->mapped_ptr = ptr;
    memory->mapped_offset = 0;
    memory->mapped_size = memory->size;

    *ppData = (uint8_t*)ptr + offset;
    return VK_SUCCESS;
}

void vkUnmapMemory(VkDevice device, VkDeviceMemory memory) {
    if (!memory || !memory->mapped_ptr) return;

    gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, memory->gl_buffer);
    gl.UnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    memory->mapped_ptr = nullptr;
    memory->mapped_offset = 0;
    memory->mapped_size = 0;
}

VkResult vkFlushMappedMemoryRanges(VkDevice device,
                                   uint32_t memoryRangeCount,
                                   const VkMappedMemoryRange* pMemoryRanges) {
    if (gl.MemoryBarrier) {
        gl.MemoryBarrier(GL_ALL_BARRIER_BITS);
    }
    return VK_SUCCESS;
}

VkResult vkInvalidateMappedMemoryRanges(VkDevice device,
                                       uint32_t memoryRangeCount,
                                       const VkMappedMemoryRange* pMemoryRanges) {
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
