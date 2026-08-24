#include "glvk_internal.hpp"
#include <cstring>

extern "C" {

VkResult vkCreateBuffer(VkDevice device,
                        const VkBufferCreateInfo* pCreateInfo,
                        const VkAllocationCallbacks* pAllocator,
                        VkBuffer* pBuffer) {
    if (!device || !pCreateInfo || !pBuffer) return VK_ERROR_INITIALIZATION_FAILED;

    auto buf = new VkBuffer_T();
    buf->size = pCreateInfo->size;
    buf->usage = pCreateInfo->usage;
    buf->memory = VK_NULL_HANDLE;
    buf->memory_offset = 0;

    *pBuffer = buf;
    return VK_SUCCESS;
}

void vkDestroyBuffer(VkDevice device,
                     VkBuffer buffer,
                     const VkAllocationCallbacks* pAllocator) {
    if (!buffer) return;
    delete buffer;
}

void vkGetBufferMemoryRequirements(VkDevice device,
                                   VkBuffer buffer,
                                   VkMemoryRequirements* pMemoryRequirements) {
    if (!buffer || !pMemoryRequirements) return;

    memset(pMemoryRequirements, 0, sizeof(VkMemoryRequirements));
    uint32_t align = 256;
    if (device && device->physical_device) {
        align = device->physical_device->gpu_info.min_ssbo_offset_alignment;
    }
    pMemoryRequirements->alignment = align;

    // Align size up to the required alignment
    VkDeviceSize aligned_size = (buffer->size + align - 1) & ~(align - 1);
    pMemoryRequirements->size = aligned_size > 0 ? aligned_size : align;
    pMemoryRequirements->memoryTypeBits = 0x3; // Matches both memory types
}

void vkGetBufferMemoryRequirements2(VkDevice device,
                                    const VkBufferMemoryRequirementsInfo2* pInfo,
                                    VkMemoryRequirements2* pMemoryRequirements) {
    if (!pInfo || !pMemoryRequirements) return;
    vkGetBufferMemoryRequirements(device, pInfo->buffer, &pMemoryRequirements->memoryRequirements);
}

void vkGetBufferMemoryRequirements2KHR(VkDevice device,
                                       const VkBufferMemoryRequirementsInfo2* pInfo,
                                       VkMemoryRequirements2* pMemoryRequirements) {
    vkGetBufferMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

} // extern "C"
