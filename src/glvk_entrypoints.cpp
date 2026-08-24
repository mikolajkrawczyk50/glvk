#include "glvk_internal.hpp"
#include <cstring>
#include <unordered_map>

extern "C" {

VkResult vkEnumerateInstanceVersion(uint32_t* pApiVersion) {
    if (!pApiVersion) return VK_ERROR_INITIALIZATION_FAILED;
    *pApiVersion = VK_API_VERSION_1_2;
    return VK_SUCCESS;
}

#define EXPORT_PROC(name) if (strcmp(pName, #name) == 0) return (PFN_vkVoidFunction)&name;

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    if (!pName) return nullptr;

    EXPORT_PROC(vkCreateInstance);
    EXPORT_PROC(vkDestroyInstance);
    EXPORT_PROC(vkEnumeratePhysicalDevices);
    EXPORT_PROC(vkGetPhysicalDeviceProperties);
    EXPORT_PROC(vkGetPhysicalDeviceProperties2);
    EXPORT_PROC(vkGetPhysicalDeviceFeatures);
    EXPORT_PROC(vkGetPhysicalDeviceFeatures2);
    EXPORT_PROC(vkGetPhysicalDeviceQueueFamilyProperties);
    EXPORT_PROC(vkGetPhysicalDeviceQueueFamilyProperties2);
    EXPORT_PROC(vkGetPhysicalDeviceMemoryProperties);
    EXPORT_PROC(vkGetPhysicalDeviceMemoryProperties2);
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
    EXPORT_PROC(vkCreateBuffer);
    EXPORT_PROC(vkDestroyBuffer);
    EXPORT_PROC(vkGetBufferMemoryRequirements);
    EXPORT_PROC(vkGetBufferMemoryRequirements2);
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
    EXPORT_PROC(vkCreateCommandPool);
    EXPORT_PROC(vkDestroyCommandPool);
    EXPORT_PROC(vkResetCommandPool);
    EXPORT_PROC(vkTrimCommandPool);
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
