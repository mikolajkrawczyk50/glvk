#include "glvk_internal.hpp"
#include <iostream>
#include <cstring>

extern "C" {

VkResult vkCreateFence(VkDevice device,
                       const VkFenceCreateInfo* pCreateInfo,
                       const VkAllocationCallbacks* pAllocator,
                       VkFence* pFence) {
    if (!device || !pCreateInfo || !pFence) return VK_ERROR_INITIALIZATION_FAILED;

    auto f = new VkFence_T();
    f->signaled = (pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT) != 0;
    f->sync = nullptr;

    *pFence = f;
    return VK_SUCCESS;
}

void vkDestroyFence(VkDevice device,
                    VkFence fence,
                    const VkAllocationCallbacks* pAllocator) {
    if (!fence) return;
    if (fence->sync) {
        GLVKContextScope scope;
        gl.DeleteSync(fence->sync);
        fence->sync = nullptr;
    }
    delete fence;
}

VkResult vkResetFences(VkDevice device,
                       uint32_t fenceCount,
                       const VkFence* pFences) {
    if (!pFences) return VK_ERROR_INITIALIZATION_FAILED;

    GLVKContextScope scope;
    for (uint32_t i = 0; i < fenceCount; i++) {
        auto f = pFences[i];
        if (f) {
            if (f->sync) {
                gl.DeleteSync(f->sync);
                f->sync = nullptr;
            }
            f->signaled = false;
        }
    }
    return VK_SUCCESS;
}

VkResult vkGetFenceStatus(VkDevice device, VkFence fence) {
    if (!fence) return VK_ERROR_INITIALIZATION_FAILED;
    if (fence->signaled) return VK_SUCCESS;
    if (!fence->sync) return VK_NOT_READY;

    GLVKContextScope scope;
    GLenum res = gl.ClientWaitSync(fence->sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
    if (res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED) {
        fence->signaled = true;
        return VK_SUCCESS;
    }
    return VK_NOT_READY;
}

VkResult vkWaitForFences(VkDevice device,
                         uint32_t fenceCount,
                         const VkFence* pFences,
                         VkBool32 waitAll,
                         uint64_t timeout) {
    if (!pFences || fenceCount == 0) return VK_SUCCESS;

    GLVKContextScope scope;

    for (uint32_t i = 0; i < fenceCount; i++) {
        auto f = pFences[i];
        if (!f) continue;
        if (f->signaled) continue;

        if (gl.MemoryBarrier) {
            gl.MemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
        }

        if (f->sync && gl.ClientWaitSync) {
            GLenum res = gl.ClientWaitSync(f->sync, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
            if (res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED) {
                f->signaled = true;
            } else {
                glFinish();
                f->signaled = true;
            }
        } else {
            glFinish();
            f->signaled = true;
        }
    }

    return VK_SUCCESS;
}

VkResult vkCreateSemaphore(VkDevice device,
                           const VkSemaphoreCreateInfo* pCreateInfo,
                           const VkAllocationCallbacks* pAllocator,
                           VkSemaphore* pSemaphore) {
    if (!device || !pSemaphore) return VK_ERROR_INITIALIZATION_FAILED;
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
    if (!device || !pCreateInfo || !pQueryPool) return VK_ERROR_INITIALIZATION_FAILED;
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

VkResult vkGetQueryPoolResults(VkDevice device,
                               VkQueryPool queryPool,
                               uint32_t firstQuery,
                               uint32_t queryCount,
                               size_t dataSize,
                               void* pData,
                               VkDeviceSize stride,
                               VkQueryResultFlags flags) {
    if (pData && dataSize >= sizeof(uint64_t) * queryCount) {
        memset(pData, 0, dataSize);
    }
    return VK_SUCCESS;
}

} // extern "C"
