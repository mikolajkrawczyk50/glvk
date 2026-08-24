#include "glvk_internal.hpp"
#include <iostream>
#include <cstring>

extern "C" {

VkResult vkCreateFence(VkDevice device,
                       const VkFenceCreateInfo* pCreateInfo,
                       const VkAllocationCallbacks* pAllocator,
                       VkFence* pFence) {
    if (!device || !pCreateInfo || !pFence) return VK_ERROR_INITIALIZATION_FAILED;

    auto fence = new VkFence_T();
    fence->signaled = (pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT) != 0;
    fence->sync = nullptr;

    *pFence = fence;
    return VK_SUCCESS;
}

void vkDestroyFence(VkDevice device,
                    VkFence fence,
                    const VkAllocationCallbacks* pAllocator) {
    if (!fence) return;
    if (fence->sync) {
        gl.DeleteSync(fence->sync);
        fence->sync = nullptr;
    }
    delete fence;
}

VkResult vkResetFences(VkDevice device,
                       uint32_t fenceCount,
                       const VkFence* pFences) {
    for (uint32_t i = 0; i < fenceCount; i++) {
        if (pFences[i]) {
            if (pFences[i]->sync) {
                gl.DeleteSync(pFences[i]->sync);
                pFences[i]->sync = nullptr;
            }
            pFences[i]->signaled = false;
        }
    }
    return VK_SUCCESS;
}

VkResult vkGetFenceStatus(VkDevice device, VkFence fence) {
    if (!fence) return VK_ERROR_INITIALIZATION_FAILED;
    if (fence->signaled) return VK_SUCCESS;

    if (fence->sync) {
        GLenum res = gl.ClientWaitSync(fence->sync, 0, 0);
        if (res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED) {
            fence->signaled = true;
            return VK_SUCCESS;
        }
    }
    return VK_NOT_READY;
}

VkResult vkWaitForFences(VkDevice device,
                         uint32_t fenceCount,
                         const VkFence* pFences,
                         VkBool32 waitAll,
                         uint64_t timeout) {
    for (uint32_t i = 0; i < fenceCount; i++) {
        auto fence = pFences[i];
        if (!fence) continue;

        if (fence->signaled) continue;

        if (fence->sync) {
            GLbitfield flags = GL_SYNC_FLUSH_COMMANDS_BIT;
            GLuint64 gl_timeout = (timeout == UINT64_MAX) ? GL_TIMEOUT_IGNORED : (GLuint64)timeout;
            GLenum res = gl.ClientWaitSync(fence->sync, flags, gl_timeout);
            if (res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED) {
                fence->signaled = true;
            } else if (res == GL_TIMEOUT_EXPIRED) {
                return VK_TIMEOUT;
            } else {
                return VK_ERROR_DEVICE_LOST;
            }
        } else {
            fence->signaled = true;
        }
    }
    return VK_SUCCESS;
}

VkResult vkCreateSemaphore(VkDevice device,
                           const VkSemaphoreCreateInfo* pCreateInfo,
                           const VkAllocationCallbacks* pAllocator,
                           VkSemaphore* pSemaphore) {
    if (!pSemaphore) return VK_ERROR_INITIALIZATION_FAILED;
    *pSemaphore = new VkSemaphore_T();
    return VK_SUCCESS;
}

void vkDestroySemaphore(VkDevice device,
                        VkSemaphore semaphore,
                        const VkAllocationCallbacks* pAllocator) {
    if (!semaphore) return;
    delete semaphore;
}

VkResult vkCreateQueryPool(VkDevice device,
                           const VkQueryPoolCreateInfo* pCreateInfo,
                           const VkAllocationCallbacks* pAllocator,
                           VkQueryPool* pQueryPool) {
    if (!pQueryPool) return VK_ERROR_INITIALIZATION_FAILED;
    auto qp = new VkQueryPool_T();
    qp->count = pCreateInfo->queryCount;
    *pQueryPool = qp;
    return VK_SUCCESS;
}

void vkDestroyQueryPool(VkDevice device,
                        VkQueryPool queryPool,
                        const VkAllocationCallbacks* pAllocator) {
    if (!queryPool) return;
    delete queryPool;
}

void vkCmdResetQueryPool(VkCommandBuffer commandBuffer,
                         VkQueryPool queryPool,
                         uint32_t firstQuery,
                         uint32_t queryCount) {
}

void vkCmdWriteTimestamp(VkCommandBuffer commandBuffer,
                         VkPipelineStageFlagBits pipelineStage,
                         VkQueryPool queryPool,
                         uint32_t query) {
}

VkResult vkGetQueryPoolResults(VkDevice device,
                               VkQueryPool queryPool,
                               uint32_t firstQuery,
                               uint32_t queryCount,
                               size_t dataSize,
                               void* pData,
                               VkDeviceSize stride,
                               VkQueryResultFlags flags) {
    if (pData && dataSize > 0) {
        memset(pData, 0, dataSize);
    }
    return VK_SUCCESS;
}

} // extern "C"
