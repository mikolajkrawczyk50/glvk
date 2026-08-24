#include "glvk_internal.hpp"
#include <cstring>
#include <iostream>

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
    pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

    strncpy(pProperties->deviceName, physicalDevice->gpu_info.device_name.c_str(), sizeof(pProperties->deviceName) - 1);

    // Compute limits
    auto& limits = pProperties->limits;
    limits.maxComputeSharedMemorySize = physicalDevice->gpu_info.max_compute_shared_memory_size;
    limits.maxComputeWorkGroupInvocations = physicalDevice->gpu_info.max_compute_workgroup_invocations;
    limits.maxComputeWorkGroupCount[0] = physicalDevice->gpu_info.max_compute_workgroup_count[0];
    limits.maxComputeWorkGroupCount[1] = physicalDevice->gpu_info.max_compute_workgroup_count[1];
    limits.maxComputeWorkGroupCount[2] = physicalDevice->gpu_info.max_compute_workgroup_count[2];
    limits.maxComputeWorkGroupSize[0] = physicalDevice->gpu_info.max_compute_workgroup_size[0];
    limits.maxComputeWorkGroupSize[1] = physicalDevice->gpu_info.max_compute_workgroup_size[1];
    limits.maxComputeWorkGroupSize[2] = physicalDevice->gpu_info.max_compute_workgroup_size[2];

    limits.minStorageBufferOffsetAlignment = physicalDevice->gpu_info.min_ssbo_offset_alignment;
    limits.minUniformBufferOffsetAlignment = 256;
    limits.maxStorageBufferRange = (uint32_t)std::min<uint64_t>(physicalDevice->gpu_info.max_ssbo_size, 0xFFFFFFFFu);
    limits.maxUniformBufferRange = 65536;
    limits.maxPushConstantsSize = 128;
    limits.maxDescriptorSetStorageBuffers = physicalDevice->gpu_info.max_ssbo_bindings;
    limits.maxBoundDescriptorSets = 4;
}

void vkGetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,
                                    VkPhysicalDeviceProperties2* pProperties) {
    if (!pProperties) return;
    vkGetPhysicalDeviceProperties(physicalDevice, &pProperties->properties);
}

void vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice,
                                 VkPhysicalDeviceFeatures* pFeatures) {
    if (!pFeatures) return;
    memset(pFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
    pFeatures->shaderInt16 = VK_FALSE;
    pFeatures->shaderInt64 = VK_TRUE;
    pFeatures->shaderFloat64 = VK_TRUE;
}

void vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice,
                                  VkPhysicalDeviceFeatures2* pFeatures) {
    if (!pFeatures) return;
    vkGetPhysicalDeviceFeatures(physicalDevice, &pFeatures->features);
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
        pQueueFamilyProperties[0].queueFlags = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
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

    // Expose two memory types: Device Local & Host Visible / Coherent
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

VkResult vkEnumerateInstanceExtensionProperties(const char* pLayerName,
                                                uint32_t* pPropertyCount,
                                                VkExtensionProperties* pProperties) {
    if (!pPropertyCount) return VK_ERROR_INITIALIZATION_FAILED;
    *pPropertyCount = 0;
    return VK_SUCCESS;
}

VkResult vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount,
                                            VkLayerProperties* pProperties) {
    if (!pPropertyCount) return VK_ERROR_INITIALIZATION_FAILED;
    *pPropertyCount = 0;
    return VK_SUCCESS;
}

VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                              const char* pLayerName,
                                              uint32_t* pPropertyCount,
                                              VkExtensionProperties* pProperties) {
    if (!pPropertyCount) return VK_ERROR_INITIALIZATION_FAILED;
    *pPropertyCount = 0;
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
