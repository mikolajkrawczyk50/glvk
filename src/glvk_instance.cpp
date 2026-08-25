#include "glvk_internal.hpp"
#include <cstring>
#include <iostream>
#include <vector>

extern "C" {

VkResult vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                          const VkAllocationCallbacks* pAllocator,
                          VkInstance* pInstance) {
    if (!pInstance) return VK_ERROR_INITIALIZATION_FAILED;

    if (!GLBackend::Instance().Initialize()) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    auto inst = new VkInstance_T();
    auto phys = new VkPhysicalDevice_T();
    phys->instance = inst;
    phys->gpu_info = GLBackend::Instance().GetGPUInfo();
    inst->physical_devices.push_back(phys);

    *pInstance = inst;
    return VK_SUCCESS;
}

void vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
    if (!instance) return;
    for (auto phys : instance->physical_devices) {
        delete phys;
    }
    delete instance;
    GLBackend::Instance().Shutdown();
}

VkResult vkEnumeratePhysicalDevices(VkInstance instance,
                                   uint32_t* pPhysicalDeviceCount,
                                   VkPhysicalDevice* pPhysicalDevices) {
    if (!instance || !pPhysicalDeviceCount) return VK_ERROR_INITIALIZATION_FAILED;

    uint32_t count = (uint32_t)instance->physical_devices.size();
    if (!pPhysicalDevices) {
        *pPhysicalDeviceCount = count;
        return VK_SUCCESS;
    }

    uint32_t to_copy = std::min(*pPhysicalDeviceCount, count);
    for (uint32_t i = 0; i < to_copy; i++) {
        pPhysicalDevices[i] = instance->physical_devices[i];
    }
    *pPhysicalDeviceCount = to_copy;
    return (to_copy < count) ? VK_INCOMPLETE : VK_SUCCESS;
}

void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice,
                                   VkPhysicalDeviceProperties* pProperties) {
    if (!physicalDevice || !pProperties) return;

    memset(pProperties, 0, sizeof(VkPhysicalDeviceProperties));
    pProperties->apiVersion = VK_API_VERSION_1_2;
    pProperties->driverVersion = VK_MAKE_VERSION(1, 0, 0);
    pProperties->vendorID = physicalDevice->gpu_info.vendor_id;
    pProperties->deviceID = physicalDevice->gpu_info.device_id;
    pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    strncpy(pProperties->deviceName, physicalDevice->gpu_info.device_name.c_str(), sizeof(pProperties->deviceName) - 1);

    // Compute limits
    auto& limits = pProperties->limits;
    limits.maxImageDimension1D = 8192;
    limits.maxImageDimension2D = 8192;
    limits.maxImageDimension3D = 2048;
    limits.maxImageArrayLayers = 2048;
    limits.maxTexelBufferElements = 65536;
    limits.maxUniformBufferRange = 65536;
    limits.maxStorageBufferRange = (uint32_t)std::min<uint64_t>(physicalDevice->gpu_info.max_ssbo_size, 0xFFFFFFFFu);
    limits.maxPushConstantsSize = 128;
    limits.maxMemoryAllocationCount = 4096;
    limits.maxSamplerAllocationCount = 4000;
    limits.bufferImageGranularity = 64;
    limits.sparseAddressSpaceSize = 0;
    limits.maxBoundDescriptorSets = 4;
    limits.maxPerStageDescriptorSamplers = 16;
    limits.maxPerStageDescriptorUniformBuffers = 12;
    limits.maxPerStageDescriptorStorageBuffers = physicalDevice->gpu_info.max_ssbo_bindings;
    limits.maxPerStageDescriptorSampledImages = 16;
    limits.maxPerStageDescriptorStorageImages = 8;
    limits.maxPerStageDescriptorInputAttachments = 4;
    limits.maxPerStageResources = 128;
    limits.maxDescriptorSetSamplers = 64;
    limits.maxDescriptorSetUniformBuffers = 64;
    limits.maxDescriptorSetStorageBuffers = physicalDevice->gpu_info.max_ssbo_bindings;
    limits.maxDescriptorSetSampledImages = 64;
    limits.maxDescriptorSetStorageImages = 64;

    limits.maxComputeSharedMemorySize = physicalDevice->gpu_info.max_compute_shared_memory_size;
    limits.maxComputeWorkGroupInvocations = physicalDevice->gpu_info.max_compute_workgroup_invocations;
    limits.maxComputeWorkGroupCount[0] = physicalDevice->gpu_info.max_compute_workgroup_count[0];
    limits.maxComputeWorkGroupCount[1] = physicalDevice->gpu_info.max_compute_workgroup_count[1];
    limits.maxComputeWorkGroupCount[2] = physicalDevice->gpu_info.max_compute_workgroup_count[2];
    limits.maxComputeWorkGroupSize[0] = physicalDevice->gpu_info.max_compute_workgroup_size[0];
    limits.maxComputeWorkGroupSize[1] = physicalDevice->gpu_info.max_compute_workgroup_size[1];
    limits.maxComputeWorkGroupSize[2] = physicalDevice->gpu_info.max_compute_workgroup_size[2];

    limits.minStorageBufferOffsetAlignment = physicalDevice->gpu_info.min_ssbo_offset_alignment > 0 ? physicalDevice->gpu_info.min_ssbo_offset_alignment : 256;
    limits.minUniformBufferOffsetAlignment = 256;
    limits.minMemoryMapAlignment = 64;
    limits.minTexelBufferOffsetAlignment = 256;
    limits.minTexelOffset = -8;
    limits.maxTexelOffset = 7;
    limits.minTexelGatherOffset = -8;
    limits.maxTexelGatherOffset = 7;
    limits.minInterpolationOffset = -0.5f;
    limits.maxInterpolationOffset = 0.5f - (1.0f / 512.0f);
    limits.subPixelInterpolationOffsetBits = 4;
    limits.maxFramebufferWidth = 8192;
    limits.maxFramebufferHeight = 8192;
    limits.maxFramebufferLayers = 1024;
    limits.framebufferColorSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    limits.framebufferDepthSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    limits.framebufferStencilSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    limits.framebufferNoAttachmentsSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    limits.maxColorAttachments = 8;
    limits.sampledImageColorSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    limits.sampledImageIntegerSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    limits.sampledImageDepthSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    limits.sampledImageStencilSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    limits.storageImageSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    limits.maxSampleMaskWords = 1;
    limits.timestampComputeAndGraphics = VK_TRUE;
    limits.timestampPeriod = 1.0f;
    limits.maxClipDistances = 8;
    limits.maxCullDistances = 8;
    limits.maxCombinedClipAndCullDistances = 8;
    limits.discreteQueuePriorities = 2;
    limits.pointSizeRange[0] = 1.0f;
    limits.pointSizeRange[1] = 64.0f;
    limits.lineWidthRange[0] = 1.0f;
    limits.lineWidthRange[1] = 64.0f;
    limits.pointSizeGranularity = 1.0f;
    limits.lineWidthGranularity = 1.0f;
    limits.strictLines = VK_FALSE;
    limits.standardSampleLocations = VK_TRUE;
    limits.optimalBufferCopyOffsetAlignment = 64;
    limits.optimalBufferCopyRowPitchAlignment = 64;
    limits.nonCoherentAtomSize = 64;
}

void vkGetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,
                                    VkPhysicalDeviceProperties2* pProperties) {
    if (!pProperties) return;
    vkGetPhysicalDeviceProperties(physicalDevice, &pProperties->properties);

    VkBaseOutStructure* next = (VkBaseOutStructure*)pProperties->pNext;
    while (next) {
        if (next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES) {
            auto* subgroup = (VkPhysicalDeviceSubgroupProperties*)next;
            subgroup->subgroupSize = 0;
            subgroup->supportedStages = 0;
            subgroup->supportedOperations = 0;
            subgroup->quadOperationsInAllStages = VK_FALSE;
        } else if (next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES_KHR) {
            auto* driver = (VkPhysicalDeviceDriverPropertiesKHR*)next;
            if (physicalDevice->gpu_info.vendor_id == 0x10DE) {
                driver->driverID = VK_DRIVER_ID_NVIDIA_PROPRIETARY;
            } else if (physicalDevice->gpu_info.vendor_id == 0x1002) {
                driver->driverID = VK_DRIVER_ID_MESA_RADV;
            } else {
                driver->driverID = VK_DRIVER_ID_MESA_LLVMPIPE;
            }
            strncpy(driver->driverName, "GLVK", sizeof(driver->driverName) - 1);
            strncpy(driver->driverInfo, "GLVK OpenGL Compute Shim", sizeof(driver->driverInfo) - 1);
            driver->conformanceVersion = { 1, 2, 0, 0 };
        }
        next = next->pNext;
    }
}

void vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice,
                                 VkPhysicalDeviceFeatures* pFeatures) {
    if (!pFeatures) return;
    memset(pFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
    pFeatures->shaderInt16 = VK_TRUE;
    pFeatures->shaderInt64 = VK_TRUE;
    pFeatures->shaderFloat64 = VK_TRUE;
}

void vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice,
                                  VkPhysicalDeviceFeatures2* pFeatures) {
    if (!pFeatures) return;
    vkGetPhysicalDeviceFeatures(physicalDevice, &pFeatures->features);

    VkBaseOutStructure* next = (VkBaseOutStructure*)pFeatures->pNext;
    while (next) {
        if (next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR) {
            auto* f = (VkPhysicalDevice8BitStorageFeaturesKHR*)next;
            f->storageBuffer8BitAccess = VK_FALSE;
            f->uniformAndStorageBuffer8BitAccess = VK_FALSE;
            f->storagePushConstant8 = VK_FALSE;
        } else if (next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR) {
            auto* f = (VkPhysicalDevice16BitStorageFeaturesKHR*)next;
            f->storageBuffer16BitAccess = VK_FALSE;
            f->uniformAndStorageBuffer16BitAccess = VK_FALSE;
            f->storagePushConstant16 = VK_FALSE;
            f->storageInputOutput16 = VK_FALSE;
        } else if (next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR) {
            auto* f = (VkPhysicalDeviceFloat16Int8FeaturesKHR*)next;
            f->shaderFloat16 = VK_FALSE;
            f->shaderInt8 = VK_FALSE;
        }
        next = next->pNext;
    }
}

void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                              uint32_t* pQueueFamilyPropertyCount,
                                              VkQueueFamilyProperties* pQueueFamilyProperties) {
    if (!pQueueFamilyPropertyCount) return;
    if (!pQueueFamilyProperties) {
        *pQueueFamilyPropertyCount = 1;
        return;
    }

    if (*pQueueFamilyPropertyCount >= 1) {
        memset(&pQueueFamilyProperties[0], 0, sizeof(VkQueueFamilyProperties));
        pQueueFamilyProperties[0].queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        pQueueFamilyProperties[0].queueCount = 1;
        pQueueFamilyProperties[0].timestampValidBits = 64;
        pQueueFamilyProperties[0].minImageTransferGranularity = { 1, 1, 1 };
        *pQueueFamilyPropertyCount = 1;
    }
}

void vkGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice,
                                               uint32_t* pQueueFamilyPropertyCount,
                                               VkQueueFamilyProperties2* pQueueFamilyProperties) {
    if (!pQueueFamilyPropertyCount) return;
    if (!pQueueFamilyProperties) {
        *pQueueFamilyPropertyCount = 1;
        return;
    }
    if (*pQueueFamilyPropertyCount >= 1) {
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, &pQueueFamilyProperties[0].queueFamilyProperties);
    }
}

void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice,
                                         VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
    if (!pMemoryProperties) return;
    memset(pMemoryProperties, 0, sizeof(VkPhysicalDeviceMemoryProperties));

    pMemoryProperties->memoryTypeCount = 2;
    pMemoryProperties->memoryTypes[0].propertyFlags = 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    pMemoryProperties->memoryTypes[0].heapIndex = 0;

    pMemoryProperties->memoryTypes[1].propertyFlags = 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    pMemoryProperties->memoryTypes[1].heapIndex = 0;

    pMemoryProperties->memoryHeapCount = 1;
    pMemoryProperties->memoryHeaps[0].size = physicalDevice->gpu_info.total_memory;
    pMemoryProperties->memoryHeaps[0].flags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
}

void vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice,
                                          VkPhysicalDeviceMemoryProperties2* pMemoryProperties) {
    if (!pMemoryProperties) return;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &pMemoryProperties->memoryProperties);
}

void vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice,
                                         VkFormat format,
                                         VkFormatProperties* pFormatProperties) {
    if (!pFormatProperties) return;
    memset(pFormatProperties, 0, sizeof(VkFormatProperties));
    pFormatProperties->optimalTilingFeatures = 
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | 
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | 
        VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | 
        VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    pFormatProperties->bufferFeatures = 
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT | 
        VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;
}

void vkGetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice,
                                          VkFormat format,
                                          VkFormatProperties2* pFormatProperties) {
    if (!pFormatProperties) return;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &pFormatProperties->formatProperties);
}

VkResult vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice,
                                                  VkFormat format,
                                                  VkImageType type,
                                                  VkImageTiling tiling,
                                                  VkImageUsageFlags usage,
                                                  VkImageCreateFlags flags,
                                                  VkImageFormatProperties* pImageFormatProperties) {
    if (!pImageFormatProperties) return VK_ERROR_INITIALIZATION_FAILED;
    memset(pImageFormatProperties, 0, sizeof(VkImageFormatProperties));
    pImageFormatProperties->maxExtent = { 8192, 8192, 8192 };
    pImageFormatProperties->maxMipLevels = 1;
    pImageFormatProperties->maxArrayLayers = 1;
    pImageFormatProperties->sampleCounts = VK_SAMPLE_COUNT_1_BIT;
    pImageFormatProperties->maxResourceSize = 256 * 1024 * 1024;
    return VK_SUCCESS;
}

VkResult vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice,
                                                   const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
                                                   VkImageFormatProperties2* pImageFormatProperties) {
    if (!pImageFormatProperties) return VK_ERROR_INITIALIZATION_FAILED;
    return vkGetPhysicalDeviceImageFormatProperties(
        physicalDevice,
        pImageFormatInfo->format,
        pImageFormatInfo->type,
        pImageFormatInfo->tiling,
        pImageFormatInfo->usage,
        pImageFormatInfo->flags,
        &pImageFormatProperties->imageFormatProperties
    );
}

static const VkExtensionProperties s_instance_extensions[] = {
    { "VK_KHR_get_physical_device_properties2", 2 },
    { "VK_KHR_external_memory_capabilities", 1 }
};

VkResult vkEnumerateInstanceExtensionProperties(const char* pLayerName,
                                                uint32_t* pPropertyCount,
                                                VkExtensionProperties* pProperties) {
    if (!pPropertyCount) return VK_ERROR_INITIALIZATION_FAILED;
    uint32_t count = sizeof(s_instance_extensions) / sizeof(s_instance_extensions[0]);
    if (!pProperties) {
        *pPropertyCount = count;
        return VK_SUCCESS;
    }
    uint32_t to_copy = std::min(*pPropertyCount, count);
    memcpy(pProperties, s_instance_extensions, to_copy * sizeof(VkExtensionProperties));
    *pPropertyCount = to_copy;
    return VK_SUCCESS;
}

VkResult vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount,
                                            VkLayerProperties* pProperties) {
    if (!pPropertyCount) return VK_ERROR_INITIALIZATION_FAILED;
    *pPropertyCount = 0;
    return VK_SUCCESS;
}

static const VkExtensionProperties s_device_extensions[] = {
    { "VK_KHR_maintenance1", 2 },
    { "VK_KHR_maintenance2", 1 },
    { "VK_KHR_maintenance3", 1 },
    { "VK_KHR_bind_memory2", 1 },
    { "VK_KHR_get_memory_requirements2", 1 },
    { "VK_KHR_storage_buffer_storage_class", 1 }
};

VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                              const char* pLayerName,
                                              uint32_t* pPropertyCount,
                                              VkExtensionProperties* pProperties) {
    if (!pPropertyCount) return VK_ERROR_INITIALIZATION_FAILED;
    uint32_t count = sizeof(s_device_extensions) / sizeof(s_device_extensions[0]);
    if (!pProperties) {
        *pPropertyCount = count;
        return VK_SUCCESS;
    }
    uint32_t to_copy = std::min(*pPropertyCount, count);
    memcpy(pProperties, s_device_extensions, to_copy * sizeof(VkExtensionProperties));
    *pPropertyCount = to_copy;
    return VK_SUCCESS;
}

VkResult vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
                                          uint32_t* pPropertyCount,
                                          VkLayerProperties* pProperties) {
    if (!pPropertyCount) return VK_ERROR_INITIALIZATION_FAILED;
    *pPropertyCount = 0;
    return VK_SUCCESS;
}

} // extern "C"
