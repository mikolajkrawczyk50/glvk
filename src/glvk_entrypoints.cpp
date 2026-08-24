#include "glvk_internal.hpp"
#include <cstring>
#include <iostream>

extern "C" {

VkResult vkEnumerateInstanceVersion(uint32_t* pApiVersion) {
    if (!pApiVersion) return VK_ERROR_INITIALIZATION_FAILED;
    *pApiVersion = VK_API_VERSION_1_2;
    return VK_SUCCESS;
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

VkResult vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice,
                                             uint32_t queueFamilyIndex,
                                             VkSurfaceKHR surface,
                                             VkBool32* pSupported) {
    if (pSupported) *pSupported = VK_FALSE;
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice,
                                                   VkSurfaceKHR surface,
                                                   VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) {
    if (pSurfaceCapabilities) {
        memset(pSurfaceCapabilities, 0, sizeof(VkSurfaceCapabilitiesKHR));
        pSurfaceCapabilities->minImageCount = 1;
        pSurfaceCapabilities->maxImageCount = 1;
        pSurfaceCapabilities->currentExtent = { 1920, 1080 };
        pSurfaceCapabilities->minImageExtent = { 1, 1 };
        pSurfaceCapabilities->maxImageExtent = { 4096, 4096 };
        pSurfaceCapabilities->maxImageArrayLayers = 1;
        pSurfaceCapabilities->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        pSurfaceCapabilities->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        pSurfaceCapabilities->supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        pSurfaceCapabilities->supportedUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface,
                                              uint32_t* pSurfaceFormatCount,
                                              VkSurfaceFormatKHR* pSurfaceFormats) {
    if (!pSurfaceFormatCount) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pSurfaceFormats) {
        *pSurfaceFormatCount = 1;
        return VK_SUCCESS;
    }
    if (*pSurfaceFormatCount >= 1) {
        pSurfaceFormats[0].format = VK_FORMAT_B8G8R8A8_UNORM;
        pSurfaceFormats[0].colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        *pSurfaceFormatCount = 1;
    }
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice,
                                                   VkSurfaceKHR surface,
                                                   uint32_t* pPresentModeCount,
                                                   VkPresentModeKHR* pPresentModes) {
    if (!pPresentModeCount) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pPresentModes) {
        *pPresentModeCount = 1;
        return VK_SUCCESS;
    }
    if (*pPresentModeCount >= 1) {
        pPresentModes[0] = VK_PRESENT_MODE_FIFO_KHR;
        *pPresentModeCount = 1;
    }
    return VK_SUCCESS;
}

void vkDestroySurfaceKHR(VkInstance instance,
                         VkSurfaceKHR surface,
                         const VkAllocationCallbacks* pAllocator) {
}

#ifdef VK_USE_PLATFORM_XCB_KHR
VkResult vkCreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    if (pSurface) *pSurface = (VkSurfaceKHR)1;
    return VK_SUCCESS;
}
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
VkResult vkCreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    if (pSurface) *pSurface = (VkSurfaceKHR)1;
    return VK_SUCCESS;
}
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
VkResult vkCreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    if (pSurface) *pSurface = (VkSurfaceKHR)1;
    return VK_SUCCESS;
}
#endif

#ifdef VK_KHR_display
VkResult vkCreateDisplayPlaneSurfaceKHR(VkInstance instance, const VkDisplaySurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    if (pSurface) *pSurface = (VkSurfaceKHR)1;
    return VK_SUCCESS;
}
#endif

VkResult vkCreateHeadlessSurfaceEXT(VkInstance instance, const VkHeadlessSurfaceCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    if (pSurface) *pSurface = (VkSurfaceKHR)1;
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkSurfaceCapabilities2KHR* pSurfaceCapabilities) {
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, uint32_t* pSurfaceFormatCount, VkSurfaceFormat2KHR* pSurfaceFormats) {
    if (pSurfaceFormatCount) *pSurfaceFormatCount = 0;
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceSurfaceCapabilities2EXT(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilities2EXT* pSurfaceCapabilities) {
    return VK_SUCCESS;
}

VkResult vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    if (pCallback) *pCallback = (VkDebugReportCallbackEXT)1;
    return VK_SUCCESS;
}

void vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
}

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger) {
    if (pMessenger) *pMessenger = (VkDebugUtilsMessengerEXT)1;
    return VK_SUCCESS;
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator) {
}

VkResult vkGetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkDisplayPropertiesKHR* pProperties) {
    if (pPropertyCount) *pPropertyCount = 0;
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceDisplayPlanePropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkDisplayPlanePropertiesKHR* pProperties) {
    if (pPropertyCount) *pPropertyCount = 0;
    return VK_SUCCESS;
}

VkResult vkGetDisplayPlaneSupportedDisplaysKHR(VkPhysicalDevice physicalDevice, uint32_t planeIndex, uint32_t* pDisplayCount, VkDisplayKHR* pDisplays) {
    if (pDisplayCount) *pDisplayCount = 0;
    return VK_SUCCESS;
}

VkResult vkGetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, uint32_t* pPropertyCount, VkDisplayModePropertiesKHR* pProperties) {
    if (pPropertyCount) *pPropertyCount = 0;
    return VK_SUCCESS;
}

VkResult vkGetDisplayPlaneCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkDisplayModeKHR mode, uint32_t planeIndex, VkDisplayPlaneCapabilitiesKHR* pCapabilities) {
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceToolPropertiesEXT(VkPhysicalDevice physicalDevice, uint32_t* pToolCount, VkPhysicalDeviceToolProperties* pToolProperties) {
    if (pToolCount) *pToolCount = 0;
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkCooperativeMatrixPropertiesKHR* pProperties) {
    if (pPropertyCount) *pPropertyCount = 0;
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceCalibrateableTimeDomainsKHR(VkPhysicalDevice physicalDevice, uint32_t* pTimeDomainCount, VkTimeDomainKHR* pTimeDomains) {
    if (pTimeDomainCount) *pTimeDomainCount = 0;
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceFragmentShadingRatesKHR(VkPhysicalDevice physicalDevice, uint32_t* pFragmentShadingRateCount, VkPhysicalDeviceFragmentShadingRateKHR* pFragmentShadingRates) {
    if (pFragmentShadingRateCount) *pFragmentShadingRateCount = 0;
    return VK_SUCCESS;
}

void vkGetPhysicalDeviceMultisamplePropertiesEXT(VkPhysicalDevice physicalDevice, VkSampleCountFlagBits samples, VkMultisamplePropertiesEXT* pMultisampleProperties) {
}

VkResult vkGetPhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, uint32_t* pCounterCount, VkPerformanceCounterKHR* pCounters, VkPerformanceCounterDescriptionKHR* pDescriptions) {
    if (pCounterCount) *pCounterCount = 0;
    return VK_SUCCESS;
}

VkResult vkEnumeratePhysicalDeviceGroups(VkInstance instance,
                                         uint32_t* pPhysicalDeviceGroupCount,
                                         VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) {
    if (!pPhysicalDeviceGroupCount) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pPhysicalDeviceGroupProperties) {
        *pPhysicalDeviceGroupCount = 1;
        return VK_SUCCESS;
    }
    if (*pPhysicalDeviceGroupCount >= 1 && instance && !instance->physical_devices.empty()) {
        memset(&pPhysicalDeviceGroupProperties[0], 0, sizeof(VkPhysicalDeviceGroupProperties));
        pPhysicalDeviceGroupProperties[0].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
        pPhysicalDeviceGroupProperties[0].physicalDeviceCount = 1;
        pPhysicalDeviceGroupProperties[0].physicalDevices[0] = instance->physical_devices[0];
        pPhysicalDeviceGroupProperties[0].subsetAllocation = VK_FALSE;
        *pPhysicalDeviceGroupCount = 1;
    }
    return VK_SUCCESS;
}

VkResult vkEnumeratePhysicalDeviceGroupsKHR(VkInstance instance,
                                            uint32_t* pPhysicalDeviceGroupCount,
                                            VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) {
    return vkEnumeratePhysicalDeviceGroups(instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
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
    EXPORT_PROC(vkGetPhysicalDeviceSurfaceSupportKHR);
    EXPORT_PROC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    EXPORT_PROC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    EXPORT_PROC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    EXPORT_PROC(vkDestroySurfaceKHR);
#ifdef VK_USE_PLATFORM_XCB_KHR
    EXPORT_PROC(vkCreateXcbSurfaceKHR);
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    EXPORT_PROC(vkCreateXlibSurfaceKHR);
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    EXPORT_PROC(vkCreateWaylandSurfaceKHR);
#endif
#ifdef VK_KHR_display
    EXPORT_PROC(vkCreateDisplayPlaneSurfaceKHR);
#endif
    EXPORT_PROC(vkCreateHeadlessSurfaceEXT);
    EXPORT_PROC(vkGetPhysicalDeviceSurfaceCapabilities2KHR);
    EXPORT_PROC(vkGetPhysicalDeviceSurfaceFormats2KHR);
    EXPORT_PROC(vkGetPhysicalDeviceSurfaceCapabilities2EXT);
    EXPORT_PROC(vkCreateDebugReportCallbackEXT);
    EXPORT_PROC(vkDestroyDebugReportCallbackEXT);
    EXPORT_PROC(vkCreateDebugUtilsMessengerEXT);
    EXPORT_PROC(vkDestroyDebugUtilsMessengerEXT);
    EXPORT_PROC(vkGetPhysicalDeviceDisplayPropertiesKHR);
    EXPORT_PROC(vkGetPhysicalDeviceDisplayPlanePropertiesKHR);
    EXPORT_PROC(vkGetDisplayPlaneSupportedDisplaysKHR);
    EXPORT_PROC(vkGetDisplayModePropertiesKHR);
    EXPORT_PROC(vkGetDisplayPlaneCapabilitiesKHR);
    EXPORT_PROC(vkGetPhysicalDeviceToolPropertiesEXT);
    EXPORT_PROC(vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR);
    EXPORT_PROC(vkGetPhysicalDeviceCalibrateableTimeDomainsKHR);
    EXPORT_PROC(vkGetPhysicalDeviceFragmentShadingRatesKHR);
    EXPORT_PROC(vkGetPhysicalDeviceMultisamplePropertiesEXT);
    EXPORT_PROC(vkGetPhysicalDeviceQueueFamilyPerformanceQueryCountersKHR);
    EXPORT_PROC(vkEnumeratePhysicalDeviceGroups);
    EXPORT_ALIAS(vkEnumeratePhysicalDeviceGroupsKHR, vkEnumeratePhysicalDeviceGroupsKHR);
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
