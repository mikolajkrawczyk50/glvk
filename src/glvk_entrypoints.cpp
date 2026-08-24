#include "glvk_internal.hpp"
#include <cstring>
#include <unordered_map>

extern "C" {

VkResult vkEnumerateInstanceVersion(uint32_t* pApiVersion) {
    if (!pApiVersion) return VK_ERROR_INITIALIZATION_FAILED;
    *pApiVersion = VK_API_VERSION_1_2;
    return VK_SUCCESS;
}

void vkGetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice,
                                                 const VkPhysicalDeviceExternalBufferInfo* pExternalBufferInfo,
                                                 VkExternalBufferProperties* pExternalBufferProperties) {
    if (pExternalBufferProperties) {
        memset(pExternalBufferProperties, 0, sizeof(VkExternalBufferProperties));
    }
}

void vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice,
                                                    const VkPhysicalDeviceExternalBufferInfo* pExternalBufferInfo,
                                                    VkExternalBufferProperties* pExternalBufferProperties) {
    vkGetPhysicalDeviceExternalBufferProperties(physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}

void vkGetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures) {
    vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
}

void vkGetPhysicalDeviceProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2* pProperties) {
    vkGetPhysicalDeviceProperties2(physicalDevice, pProperties);
}

void vkGetPhysicalDeviceFormatProperties2KHR(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2* pFormatProperties) {
    vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, pFormatProperties);
}

VkResult vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice,
                                                      const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
                                                      VkImageFormatProperties2* pImageFormatProperties) {
    return vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

void vkGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice,
                                                  uint32_t* pQueueFamilyPropertyCount,
                                                  VkQueueFamilyProperties2* pQueueFamilyProperties) {
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void vkGetPhysicalDeviceMemoryProperties2KHR(VkPhysicalDevice physicalDevice,
                                             VkPhysicalDeviceMemoryProperties2* pMemoryProperties) {
    vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}

#define EXPORT_PROC(name) if (strcmp(pName, #name) == 0) return (PFN_vkVoidFunction)&name;
#define EXPORT_ALIAS(name, target) if (strcmp(pName, #name) == 0) return (PFN_vkVoidFunction)&target;

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    if (!pName) return nullptr;

    EXPORT_PROC(vkCreateInstance);
    EXPORT_PROC(vkDestroyInstance);
    EXPORT_PROC(vkEnumeratePhysicalDevices);
    EXPORT_PROC(vkGetPhysicalDeviceProperties);
    EXPORT_PROC(vkGetPhysicalDeviceProperties2);
    EXPORT_ALIAS(vkGetPhysicalDeviceProperties2KHR, vkGetPhysicalDeviceProperties2KHR);
    EXPORT_PROC(vkGetPhysicalDeviceFeatures);
    EXPORT_PROC(vkGetPhysicalDeviceFeatures2);
    EXPORT_ALIAS(vkGetPhysicalDeviceFeatures2KHR, vkGetPhysicalDeviceFeatures2KHR);
    EXPORT_PROC(vkGetPhysicalDeviceQueueFamilyProperties);
    EXPORT_PROC(vkGetPhysicalDeviceQueueFamilyProperties2);
    EXPORT_ALIAS(vkGetPhysicalDeviceQueueFamilyProperties2KHR, vkGetPhysicalDeviceQueueFamilyProperties2KHR);
    EXPORT_PROC(vkGetPhysicalDeviceMemoryProperties);
    EXPORT_PROC(vkGetPhysicalDeviceMemoryProperties2);
    EXPORT_ALIAS(vkGetPhysicalDeviceMemoryProperties2KHR, vkGetPhysicalDeviceMemoryProperties2KHR);
    EXPORT_PROC(vkGetPhysicalDeviceFormatProperties);
    EXPORT_PROC(vkGetPhysicalDeviceFormatProperties2);
    EXPORT_ALIAS(vkGetPhysicalDeviceFormatProperties2KHR, vkGetPhysicalDeviceFormatProperties2KHR);
    EXPORT_PROC(vkGetPhysicalDeviceImageFormatProperties);
    EXPORT_PROC(vkGetPhysicalDeviceImageFormatProperties2);
    EXPORT_ALIAS(vkGetPhysicalDeviceImageFormatProperties2KHR, vkGetPhysicalDeviceImageFormatProperties2KHR);
    EXPORT_PROC(vkGetPhysicalDeviceExternalBufferProperties);
    EXPORT_ALIAS(vkGetPhysicalDeviceExternalBufferPropertiesKHR, vkGetPhysicalDeviceExternalBufferPropertiesKHR);
    EXPORT_PROC(vkEnumerateInstanceExtensionProperties);
    EXPORT_PROC(vkEnumerateInstanceLayerProperties);
    EXPORT_PROC(vkEnumerateDeviceExtensionProperties);
    EXPORT_PROC(vkEnumerateDeviceLayerProperties);
    EXPORT_PROC(vkEnumerateInstanceVersion);
    EXPORT_PROC(vkCreateDevice);
    EXPORT_PROC(vkDestroyDevice);
    EXPORT_PROC(vkGetDeviceQueue);
    EXPORT_PROC(vkDeviceWaitIdle);
    EXPORT_PROC(vkAllocateMemory);
    EXPORT_PROC(vkFreeMemory);
    EXPORT_PROC(vkMapMemory);
    EXPORT_PROC(vkUnmapMemory);
    EXPORT_PROC(vkFlushMappedMemoryRanges);
    EXPORT_PROC(vkInvalidateMappedMemoryRanges);
    EXPORT_PROC(vkBindBufferMemory);
    EXPORT_PROC(vkBindBufferMemory2);
    EXPORT_ALIAS(vkBindBufferMemory2KHR, vkBindBufferMemory2);
    EXPORT_PROC(vkBindImageMemory);
    EXPORT_PROC(vkBindImageMemory2);
    EXPORT_ALIAS(vkBindImageMemory2KHR, vkBindImageMemory2);
    EXPORT_PROC(vkGetDeviceMemoryCommitment);
    EXPORT_PROC(vkCreateBuffer);
    EXPORT_PROC(vkDestroyBuffer);
    EXPORT_PROC(vkGetBufferMemoryRequirements);
    EXPORT_PROC(vkGetBufferMemoryRequirements2);
    EXPORT_ALIAS(vkGetBufferMemoryRequirements2KHR, vkGetBufferMemoryRequirements2);
    EXPORT_PROC(vkCreateBufferView);
    EXPORT_PROC(vkDestroyBufferView);
    EXPORT_PROC(vkCreateImage);
    EXPORT_PROC(vkDestroyImage);
    EXPORT_PROC(vkGetImageMemoryRequirements);
    EXPORT_PROC(vkGetImageMemoryRequirements2);
    EXPORT_ALIAS(vkGetImageMemoryRequirements2KHR, vkGetImageMemoryRequirements2);
    EXPORT_PROC(vkGetImageSubresourceLayout);
    EXPORT_PROC(vkCreateImageView);
    EXPORT_PROC(vkDestroyImageView);
    EXPORT_PROC(vkCreateSampler);
    EXPORT_PROC(vkDestroySampler);
    EXPORT_PROC(vkCreateShaderModule);
    EXPORT_PROC(vkDestroyShaderModule);
    EXPORT_PROC(vkCreatePipelineLayout);
    EXPORT_PROC(vkDestroyPipelineLayout);
    EXPORT_PROC(vkCreateComputePipelines);
    EXPORT_PROC(vkDestroyPipeline);
    EXPORT_PROC(vkCreatePipelineCache);
    EXPORT_PROC(vkDestroyPipelineCache);
    EXPORT_PROC(vkGetPipelineCacheData);
    EXPORT_PROC(vkMergePipelineCaches);
    EXPORT_PROC(vkCreateDescriptorSetLayout);
    EXPORT_PROC(vkDestroyDescriptorSetLayout);
    EXPORT_PROC(vkCreateDescriptorPool);
    EXPORT_PROC(vkDestroyDescriptorPool);
    EXPORT_PROC(vkResetDescriptorPool);
    EXPORT_PROC(vkAllocateDescriptorSets);
    EXPORT_PROC(vkFreeDescriptorSets);
    EXPORT_PROC(vkUpdateDescriptorSets);
    EXPORT_PROC(vkGetDescriptorSetLayoutSupport);
    EXPORT_ALIAS(vkGetDescriptorSetLayoutSupportKHR, vkGetDescriptorSetLayoutSupport);
    EXPORT_PROC(vkCreateCommandPool);
    EXPORT_PROC(vkDestroyCommandPool);
    EXPORT_PROC(vkResetCommandPool);
    EXPORT_PROC(vkTrimCommandPool);
    EXPORT_ALIAS(vkTrimCommandPoolKHR, vkTrimCommandPoolKHR);
    EXPORT_PROC(vkAllocateCommandBuffers);
    EXPORT_PROC(vkFreeCommandBuffers);
    EXPORT_PROC(vkBeginCommandBuffer);
    EXPORT_PROC(vkEndCommandBuffer);
    EXPORT_PROC(vkResetCommandBuffer);
    EXPORT_PROC(vkCmdBindPipeline);
    EXPORT_PROC(vkCmdBindDescriptorSets);
    EXPORT_PROC(vkCmdPushConstants);
    EXPORT_PROC(vkCmdDispatch);
    EXPORT_PROC(vkCmdDispatchIndirect);
    EXPORT_PROC(vkCmdPipelineBarrier);
    EXPORT_PROC(vkCmdCopyBuffer);
    EXPORT_PROC(vkCmdFillBuffer);
    EXPORT_PROC(vkCmdUpdateBuffer);
    EXPORT_PROC(vkCmdCopyBufferToImage);
    EXPORT_PROC(vkCmdCopyImage);
    EXPORT_PROC(vkCmdCopyImageToBuffer);
    EXPORT_PROC(vkCmdExecuteCommands);
    EXPORT_PROC(vkCmdResolveImage);
    EXPORT_PROC(vkCmdBindIndexBuffer);
    EXPORT_PROC(vkQueueSubmit);
    EXPORT_PROC(vkQueueWaitIdle);
    EXPORT_PROC(vkCreateFence);
    EXPORT_PROC(vkDestroyFence);
    EXPORT_PROC(vkResetFences);
    EXPORT_PROC(vkGetFenceStatus);
    EXPORT_PROC(vkWaitForFences);
    EXPORT_PROC(vkCreateSemaphore);
    EXPORT_PROC(vkDestroySemaphore);
    EXPORT_PROC(vkCreateQueryPool);
    EXPORT_PROC(vkDestroyQueryPool);
    EXPORT_PROC(vkCmdResetQueryPool);
    EXPORT_PROC(vkCmdWriteTimestamp);
    EXPORT_PROC(vkGetQueryPoolResults);
    EXPORT_PROC(vkGetInstanceProcAddr);
    EXPORT_PROC(vkGetDeviceProcAddr);

    return nullptr;
}

PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    return vkGetInstanceProcAddr(VK_NULL_HANDLE, pName);
}

} // extern "C"
