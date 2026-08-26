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
    VkDeviceSize align = 256;
    if (device && device->physical_device) {
        align = device->physical_device->gpu_info.min_ssbo_offset_alignment;
    }
    pMemoryRequirements->alignment = align;

    VkDeviceSize aligned_size = (buffer->size + align - 1) & ~(align - 1);
    pMemoryRequirements->size = aligned_size > 0 ? aligned_size : align;
    pMemoryRequirements->memoryTypeBits = 0x3;
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

VkResult vkCreateBufferView(VkDevice device,
                            const VkBufferViewCreateInfo* pCreateInfo,
                            const VkAllocationCallbacks* pAllocator,
                            VkBufferView* pView) {
    if (pView) *pView = (VkBufferView)1;
    return VK_SUCCESS;
}

void vkDestroyBufferView(VkDevice device,
                         VkBufferView bufferView,
                         const VkAllocationCallbacks* pAllocator) {
}

VkResult vkCreateImage(VkDevice device,
                       const VkImageCreateInfo* pCreateInfo,
                       const VkAllocationCallbacks* pAllocator,
                       VkImage* pImage) {
    if (pImage) *pImage = (VkImage)1;
    return VK_SUCCESS;
}

void vkDestroyImage(VkDevice device,
                    VkImage image,
                    const VkAllocationCallbacks* pAllocator) {
}

void vkGetImageMemoryRequirements(VkDevice device,
                                  VkImage image,
                                  VkMemoryRequirements* pMemoryRequirements) {
    if (!pMemoryRequirements) return;
    memset(pMemoryRequirements, 0, sizeof(VkMemoryRequirements));
    pMemoryRequirements->size = 65536;
    pMemoryRequirements->alignment = 256;
    pMemoryRequirements->memoryTypeBits = 0x3;
}

void vkGetImageMemoryRequirements2(VkDevice device,
                                   const VkImageMemoryRequirementsInfo2* pInfo,
                                   VkMemoryRequirements2* pMemoryRequirements) {
    if (!pMemoryRequirements) return;
    vkGetImageMemoryRequirements(device, pInfo->image, &pMemoryRequirements->memoryRequirements);
}

void vkGetImageSubresourceLayout(VkDevice device,
                                 VkImage image,
                                 const VkImageSubresource* pSubresource,
                                 VkSubresourceLayout* pLayout) {
    if (!pLayout) return;
    memset(pLayout, 0, sizeof(VkSubresourceLayout));
    pLayout->rowPitch = 1024;
    pLayout->depthPitch = 1024 * 1024;
    pLayout->size = 1024 * 1024 * 4;
}

VkResult vkCreateImageView(VkDevice device,
                           const VkImageViewCreateInfo* pCreateInfo,
                           const VkAllocationCallbacks* pAllocator,
                           VkImageView* pView) {
    if (pView) *pView = (VkImageView)1;
    return VK_SUCCESS;
}

void vkDestroyImageView(VkDevice device,
                        VkImageView imageView,
                        const VkAllocationCallbacks* pAllocator) {
}

VkResult vkCreateSampler(VkDevice device,
                         const VkSamplerCreateInfo* pCreateInfo,
                         const VkAllocationCallbacks* pAllocator,
                         VkSampler* pSampler) {
    if (pSampler) *pSampler = (VkSampler)1;
    return VK_SUCCESS;
}

void vkDestroySampler(VkDevice device,
                      VkSampler sampler,
                      const VkAllocationCallbacks* pAllocator) {
}

} // extern "C"
