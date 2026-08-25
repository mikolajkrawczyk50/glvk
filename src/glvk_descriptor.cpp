#include "glvk_internal.hpp"
#include <iostream>
#include <algorithm>

extern "C" {

VkResult vkCreateDescriptorSetLayout(VkDevice device,
                                     const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
                                     const VkAllocationCallbacks* pAllocator,
                                     VkDescriptorSetLayout* pSetLayout) {
    if (!device || !pCreateInfo || !pSetLayout) return VK_ERROR_INITIALIZATION_FAILED;

    auto layout = new VkDescriptorSetLayout_T();
    for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
        const auto& b = pCreateInfo->pBindings[i];
        layout->bindings.push_back({
            b.binding,
            b.descriptorType,
            b.descriptorCount,
            b.stageFlags
        });
    }

    *pSetLayout = layout;
    return VK_SUCCESS;
}

void vkDestroyDescriptorSetLayout(VkDevice device,
                                  VkDescriptorSetLayout descriptorSetLayout,
                                  const VkAllocationCallbacks* pAllocator) {
    if (!descriptorSetLayout) return;
    delete descriptorSetLayout;
}

VkResult vkCreateDescriptorPool(VkDevice device,
                                const VkDescriptorPoolCreateInfo* pCreateInfo,
                                const VkAllocationCallbacks* pAllocator,
                                VkDescriptorPool* pDescriptorPool) {
    if (!device || !pCreateInfo || !pDescriptorPool) return VK_ERROR_INITIALIZATION_FAILED;

    auto pool = new VkDescriptorPool_T();
    *pDescriptorPool = pool;
    return VK_SUCCESS;
}

void vkDestroyDescriptorPool(VkDevice device,
                             VkDescriptorPool descriptorPool,
                             const VkAllocationCallbacks* pAllocator) {
    if (!descriptorPool) return;
    for (auto set : descriptorPool->allocated_sets) {
        delete set;
    }
    delete descriptorPool;
}

VkResult vkResetDescriptorPool(VkDevice device,
                               VkDescriptorPool descriptorPool,
                               VkDescriptorPoolResetFlags flags) {
    if (!descriptorPool) return VK_SUCCESS;
    for (auto set : descriptorPool->allocated_sets) {
        delete set;
    }
    descriptorPool->allocated_sets.clear();
    return VK_SUCCESS;
}

VkResult vkAllocateDescriptorSets(VkDevice device,
                                  const VkDescriptorSetAllocateInfo* pAllocateInfo,
                                  VkDescriptorSet* pDescriptorSets) {
    if (!device || !pAllocateInfo || !pDescriptorSets) return VK_ERROR_INITIALIZATION_FAILED;

    auto pool = pAllocateInfo->descriptorPool;
    for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
        auto set = new VkDescriptorSet_T();
        set->layout = pAllocateInfo->pSetLayouts[i];
        pool->allocated_sets.push_back(set);
        pDescriptorSets[i] = set;
    }

    return VK_SUCCESS;
}

VkResult vkFreeDescriptorSets(VkDevice device,
                              VkDescriptorPool descriptorPool,
                              uint32_t descriptorSetCount,
                              const VkDescriptorSet* pDescriptorSets) {
    if (!descriptorPool || !pDescriptorSets) return VK_SUCCESS;

    for (uint32_t i = 0; i < descriptorSetCount; i++) {
        auto set = pDescriptorSets[i];
        auto it = std::find(descriptorPool->allocated_sets.begin(), descriptorPool->allocated_sets.end(), set);
        if (it != descriptorPool->allocated_sets.end()) {
            descriptorPool->allocated_sets.erase(it);
        }
        delete set;
    }
    return VK_SUCCESS;
}

void vkUpdateDescriptorSets(VkDevice device,
                            uint32_t descriptorWriteCount,
                            const VkWriteDescriptorSet* pDescriptorWrites,
                            uint32_t descriptorCopyCount,
                            const VkCopyDescriptorSet* pDescriptorCopies) {
    for (uint32_t i = 0; i < descriptorWriteCount; i++) {
        const auto& write = pDescriptorWrites[i];
        auto set = write.dstSet;
        if (!set) continue;

        if (write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
            write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
            write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
            write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            
            for (uint32_t j = 0; j < write.descriptorCount; j++) {
                const auto& bufInfo = write.pBufferInfo[j];
                if (bufInfo.buffer && bufInfo.buffer->memory) {
                    GLuint gl_buf = bufInfo.buffer->memory->gl_buffer;
                    VkDeviceSize final_offset = bufInfo.buffer->memory_offset + bufInfo.offset;
                    VkDeviceSize range = bufInfo.range;
                    if (range == VK_WHOLE_SIZE) {
                        range = bufInfo.buffer->size - bufInfo.offset;
                    }
                    set->buffer_bindings[write.dstBinding + j] = {
                        gl_buf, final_offset, range, 0,
                        bufInfo.buffer->memory->gl_buffers,
                        bufInfo.buffer->memory->bank_sizes
                    };
                }
            }
        }
    }
}

void vkGetDescriptorSetLayoutSupport(VkDevice device,
                                     const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
                                     VkDescriptorSetLayoutSupport* pSupport) {
    if (!pSupport) return;
    pSupport->supported = VK_TRUE;
}

void vkGetDescriptorSetLayoutSupportKHR(VkDevice device,
                                        const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
                                        VkDescriptorSetLayoutSupport* pSupport) {
    vkGetDescriptorSetLayoutSupport(device, pCreateInfo, pSupport);
}

} // extern "C"
