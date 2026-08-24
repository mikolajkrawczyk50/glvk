#include "glvk_internal.hpp"
#include "glvk_shader.hpp"
#include <iostream>

extern "C" {

VkResult vkCreatePipelineLayout(VkDevice device,
                                const VkPipelineLayoutCreateInfo* pCreateInfo,
                                const VkAllocationCallbacks* pAllocator,
                                VkPipelineLayout* pPipelineLayout) {
    if (!device || !pCreateInfo || !pPipelineLayout) return VK_ERROR_INITIALIZATION_FAILED;

    auto layout = new VkPipelineLayout_T();
    if (pCreateInfo->pSetLayouts && pCreateInfo->setLayoutCount > 0) {
        layout->set_layouts.assign(pCreateInfo->pSetLayouts, pCreateInfo->pSetLayouts + pCreateInfo->setLayoutCount);
    }
    if (pCreateInfo->pPushConstantRanges && pCreateInfo->pushConstantRangeCount > 0) {
        layout->push_constant_ranges.assign(pCreateInfo->pPushConstantRanges,
                                            pCreateInfo->pPushConstantRanges + pCreateInfo->pushConstantRangeCount);
    }

    *pPipelineLayout = layout;
    return VK_SUCCESS;
}

void vkDestroyPipelineLayout(VkDevice device,
                             VkPipelineLayout pipelineLayout,
                             const VkAllocationCallbacks* pAllocator) {
    if (!pipelineLayout) return;
    delete pipelineLayout;
}

VkResult vkCreateComputePipelines(VkDevice device,
                                  VkPipelineCache pipelineCache,
                                  uint32_t createInfoCount,
                                  const VkComputePipelineCreateInfo* pCreateInfos,
                                  const VkAllocationCallbacks* pAllocator,
                                  VkPipeline* pPipelines) {
    if (!device || !pCreateInfos || !pPipelines) return VK_ERROR_INITIALIZATION_FAILED;

    for (uint32_t i = 0; i < createInfoCount; i++) {
        const auto& info = pCreateInfos[i];
        if (!info.stage.module) {
            pPipelines[i] = VK_NULL_HANDLE;
            continue;
        }

        auto pipe = new VkPipeline_T();
        pipe->layout = info.layout;

        pipe->gl_program = CompileSpirvToGLProgram(
            info.stage.module->spirv_words,
            info.stage.pSpecializationInfo,
            info.layout,
            pipe->push_constant_uniforms
        );

        if (pipe->gl_program == 0) {
            delete pipe;
            pPipelines[i] = VK_NULL_HANDLE;
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        pPipelines[i] = pipe;
    }

    return VK_SUCCESS;
}

void vkDestroyPipeline(VkDevice device,
                       VkPipeline pipeline,
                       const VkAllocationCallbacks* pAllocator) {
    if (!pipeline) return;
    if (pipeline->gl_program) {
        gl.DeleteProgram(pipeline->gl_program);
        pipeline->gl_program = 0;
    }
    delete pipeline;
}

VkResult vkCreatePipelineCache(VkDevice device,
                               const VkPipelineCacheCreateInfo* pCreateInfo,
                               const VkAllocationCallbacks* pAllocator,
                               VkPipelineCache* pPipelineCache) {
    if (!pPipelineCache) return VK_ERROR_INITIALIZATION_FAILED;
    *pPipelineCache = new VkPipelineCache_T();
    return VK_SUCCESS;
}

void vkDestroyPipelineCache(VkDevice device,
                            VkPipelineCache pipelineCache,
                            const VkAllocationCallbacks* pAllocator) {
    if (!pipelineCache) return;
    delete pipelineCache;
}

VkResult vkGetPipelineCacheData(VkDevice device,
                                VkPipelineCache pipelineCache,
                                size_t* pDataSize,
                                void* pData) {
    if (!pDataSize) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pData) {
        *pDataSize = 0;
        return VK_SUCCESS;
    }
    *pDataSize = 0;
    return VK_SUCCESS;
}

VkResult vkMergePipelineCaches(VkDevice device,
                               VkPipelineCache dstCache,
                               uint32_t srcCacheCount,
                               const VkPipelineCache* pSrcCaches) {
    return VK_SUCCESS;
}

} // extern "C"
