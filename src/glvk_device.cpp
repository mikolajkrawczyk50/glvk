#include "glvk_internal.hpp"
#include <iostream>

extern "C" {

VkResult vkCreateDevice(VkPhysicalDevice physicalDevice,
                        const VkDeviceCreateInfo* pCreateInfo,
                        const VkAllocationCallbacks* pAllocator,
                        VkDevice* pDevice) {
    if (!physicalDevice || !pDevice) return VK_ERROR_INITIALIZATION_FAILED;

    auto dev = new VkDevice_T();
    dev->physical_device = physicalDevice;

    auto q = new VkQueue_T();
    q->device = dev;
    q->family_index = 0;
    q->queue_index = 0;
    dev->default_queue = q;

    *pDevice = dev;
    return VK_SUCCESS;
}

void vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
    if (!device) return;
    if (device->default_queue) {
        delete device->default_queue;
    }
    delete device;
}

void vkGetDeviceQueue(VkDevice device,
                      uint32_t queueFamilyIndex,
                      uint32_t queueIndex,
                      VkQueue* pQueue) {
    if (!device || !pQueue) return;
    *pQueue = device->default_queue;
}

VkResult vkDeviceWaitIdle(VkDevice device) {
    if (gl.ClientWaitSync) {
        // Issue memory barrier and glFinish
        if (gl.MemoryBarrier) {
            gl.MemoryBarrier(GL_ALL_BARRIER_BITS);
        }
        glFinish();
    }
    return VK_SUCCESS;
}

} // extern "C"
