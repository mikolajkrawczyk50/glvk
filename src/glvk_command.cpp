#include "glvk_internal.hpp"
#include <iostream>
#include <cstring>

extern "C" {

VkResult vkCreateCommandPool(VkDevice device,
                             const VkCommandPoolCreateInfo* pCreateInfo,
                             const VkAllocationCallbacks* pAllocator,
                             VkCommandPool* pCommandPool) {
    if (!device || !pCreateInfo || !pCommandPool) return VK_ERROR_INITIALIZATION_FAILED;

    auto pool = new VkCommandPool_T();
    pool->queue_family_index = pCreateInfo->queueFamilyIndex;
    pool->flags = pCreateInfo->flags;

    *pCommandPool = pool;
    return VK_SUCCESS;
}

void vkDestroyCommandPool(VkDevice device,
                          VkCommandPool commandPool,
                          const VkAllocationCallbacks* pAllocator) {
    if (!commandPool) return;
    delete commandPool;
}

VkResult vkResetCommandPool(VkDevice device,
                            VkCommandPool commandPool,
                            VkCommandPoolResetFlags flags) {
    return VK_SUCCESS;
}

void vkTrimCommandPool(VkDevice device,
                       VkCommandPool commandPool,
                       VkCommandPoolTrimFlags flags) {
}

void vkTrimCommandPoolKHR(VkDevice device,
                          VkCommandPool commandPool,
                          VkCommandPoolTrimFlags flags) {
}

VkResult vkAllocateCommandBuffers(VkDevice device,
                                  const VkCommandBufferAllocateInfo* pAllocateInfo,
                                  VkCommandBuffer* pCommandBuffers) {
    if (!device || !pAllocateInfo || !pCommandBuffers) return VK_ERROR_INITIALIZATION_FAILED;

    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        auto cb = new VkCommandBuffer_T();
        cb->pool = pAllocateInfo->commandPool;
        cb->level = pAllocateInfo->level;
        pCommandBuffers[i] = cb;
    }

    return VK_SUCCESS;
}

void vkFreeCommandBuffers(VkDevice device,
                          VkCommandPool commandPool,
                          uint32_t commandBufferCount,
                          const VkCommandBuffer* pCommandBuffers) {
    if (!pCommandBuffers) return;
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        delete pCommandBuffers[i];
    }
}

VkResult vkBeginCommandBuffer(VkCommandBuffer commandBuffer,
                              const VkCommandBufferBeginInfo* pBeginInfo) {
    if (!commandBuffer) return VK_ERROR_INITIALIZATION_FAILED;
    commandBuffer->recorded_commands.clear();
    commandBuffer->is_recording = true;
    return VK_SUCCESS;
}

VkResult vkEndCommandBuffer(VkCommandBuffer commandBuffer) {
    if (!commandBuffer) return VK_ERROR_INITIALIZATION_FAILED;
    commandBuffer->is_recording = false;
    return VK_SUCCESS;
}

VkResult vkResetCommandBuffer(VkCommandBuffer commandBuffer,
                              VkCommandBufferResetFlags flags) {
    if (!commandBuffer) return VK_SUCCESS;
    commandBuffer->recorded_commands.clear();
    commandBuffer->is_recording = false;
    return VK_SUCCESS;
}

void vkCmdBindPipeline(VkCommandBuffer commandBuffer,
                       VkPipelineBindPoint pipelineBindPoint,
                       VkPipeline pipeline) {
    if (!commandBuffer || !pipeline) return;

    GLVKCmd cmd;
    cmd.type = GLVKCmdType::BindPipeline;
    cmd.bind_pipeline.pipeline = pipeline;
    commandBuffer->recorded_commands.push_back(cmd);
}

void vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer,
                             VkPipelineBindPoint pipelineBindPoint,
                             VkPipelineLayout layout,
                             uint32_t firstSet,
                             uint32_t descriptorSetCount,
                             const VkDescriptorSet* pDescriptorSets,
                             uint32_t dynamicOffsetCount,
                             const uint32_t* pDynamicOffsets) {
    if (!commandBuffer || !pDescriptorSets) return;

    GLVKCmd cmd;
    cmd.type = GLVKCmdType::BindDescriptorSets;
    cmd.bind_descriptor_sets.first_set = firstSet;
    cmd.bind_descriptor_sets.descriptor_set_count = std::min(descriptorSetCount, 8u);
    for (uint32_t i = 0; i < cmd.bind_descriptor_sets.descriptor_set_count; i++) {
        cmd.bind_descriptor_sets.sets[i] = pDescriptorSets[i];
    }
    commandBuffer->recorded_commands.push_back(cmd);
}

void vkCmdPushConstants(VkCommandBuffer commandBuffer,
                        VkPipelineLayout layout,
                        VkShaderStageFlags stageFlags,
                        uint32_t offset,
                        uint32_t size,
                        const void* pValues) {
    if (!commandBuffer || !pValues || size == 0) return;

    GLVKCmd cmd;
    cmd.type = GLVKCmdType::PushConstants;
    cmd.push_constants.offset = offset;
    cmd.push_constants.size = std::min(size, 128u);
    memcpy(cmd.push_constants.data, pValues, cmd.push_constants.size);
    commandBuffer->recorded_commands.push_back(cmd);
}

void vkCmdDispatch(VkCommandBuffer commandBuffer,
                   uint32_t groupCountX,
                   uint32_t groupCountY,
                   uint32_t groupCountZ) {
    if (!commandBuffer) return;

    GLVKCmd cmd;
    cmd.type = GLVKCmdType::Dispatch;
    cmd.dispatch.group_count_x = groupCountX;
    cmd.dispatch.group_count_y = groupCountY;
    cmd.dispatch.group_count_z = groupCountZ;
    commandBuffer->recorded_commands.push_back(cmd);
}

void vkCmdDispatchIndirect(VkCommandBuffer commandBuffer,
                           VkBuffer buffer,
                           VkDeviceSize offset) {
    if (!commandBuffer || !buffer) return;

    GLVKCmd cmd;
    cmd.type = GLVKCmdType::DispatchIndirect;
    cmd.dispatch_indirect.buffer = buffer;
    cmd.dispatch_indirect.offset = offset;
    commandBuffer->recorded_commands.push_back(cmd);
}

void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer,
                          VkPipelineStageFlags srcStageMask,
                          VkPipelineStageFlags dstStageMask,
                          VkDependencyFlags dependencyFlags,
                          uint32_t memoryBarrierCount,
                          const VkMemoryBarrier* pMemoryBarriers,
                          uint32_t bufferMemoryBarrierCount,
                          const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                          uint32_t imageMemoryBarrierCount,
                          const VkImageMemoryBarrier* pImageMemoryBarriers) {
    if (!commandBuffer) return;

    GLVKCmd cmd;
    cmd.type = GLVKCmdType::PipelineBarrier;
    cmd.pipeline_barrier.src_stage_mask = srcStageMask;
    cmd.pipeline_barrier.dst_stage_mask = dstStageMask;
    commandBuffer->recorded_commands.push_back(cmd);
}

void vkCmdCopyBuffer(VkCommandBuffer commandBuffer,
                     VkBuffer srcBuffer,
                     VkBuffer dstBuffer,
                     uint32_t regionCount,
                     const VkBufferCopy* pRegions) {
    if (!commandBuffer || !srcBuffer || !dstBuffer || !pRegions) return;

    for (uint32_t i = 0; i < regionCount; i++) {
        GLVKCmd cmd;
        cmd.type = GLVKCmdType::CopyBuffer;
        cmd.copy_buffer.src_buffer = srcBuffer;
        cmd.copy_buffer.dst_buffer = dstBuffer;
        cmd.copy_buffer.src_offset = pRegions[i].srcOffset;
        cmd.copy_buffer.dst_offset = pRegions[i].dstOffset;
        cmd.copy_buffer.size = pRegions[i].size;
        commandBuffer->recorded_commands.push_back(cmd);
    }
}

void vkCmdFillBuffer(VkCommandBuffer commandBuffer,
                     VkBuffer dstBuffer,
                     VkDeviceSize dstOffset,
                     VkDeviceSize size,
                     uint32_t data) {
    if (!commandBuffer || !dstBuffer) return;

    GLVKCmd cmd;
    cmd.type = GLVKCmdType::FillBuffer;
    cmd.fill_buffer.dst_buffer = dstBuffer;
    cmd.fill_buffer.dst_offset = dstOffset;
    cmd.fill_buffer.size = size;
    cmd.fill_buffer.data = data;
    commandBuffer->recorded_commands.push_back(cmd);
}

void vkCmdUpdateBuffer(VkCommandBuffer commandBuffer,
                       VkBuffer dstBuffer,
                       VkDeviceSize dstOffset,
                       VkDeviceSize dataSize,
                       const void* pData) {
    if (!commandBuffer || !dstBuffer || !pData || dataSize == 0) return;

    if (dstBuffer->memory) {
        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, dstBuffer->memory->gl_buffer);
        gl.BufferSubData(GL_SHADER_STORAGE_BUFFER, (GLintptr)(dstBuffer->memory_offset + dstOffset), (GLsizeiptr)dataSize, pData);
        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

void vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer,
                               VkPipelineBindPoint pipelineBindPoint,
                               VkPipelineLayout layout,
                               uint32_t set,
                               uint32_t descriptorWriteCount,
                               const VkWriteDescriptorSet* pDescriptorWrites) {
}

void vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer,
                            VkBuffer srcBuffer,
                            VkImage dstImage,
                            VkImageLayout dstImageLayout,
                            uint32_t regionCount,
                            const VkBufferImageCopy* pRegions) {
}

void vkCmdCopyImage(VkCommandBuffer commandBuffer,
                    VkImage srcImage,
                    VkImageLayout srcImageLayout,
                    VkImage dstImage,
                    VkImageLayout dstImageLayout,
                    uint32_t regionCount,
                    const VkImageCopy* pRegions) {
}

void vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer,
                            VkImage srcImage,
                            VkImageLayout srcImageLayout,
                            VkBuffer dstBuffer,
                            uint32_t regionCount,
                            const VkBufferImageCopy* pRegions) {
}

void vkCmdExecuteCommands(VkCommandBuffer commandBuffer,
                          uint32_t commandBufferCount,
                          const VkCommandBuffer* pCommandBuffers) {
    if (!commandBuffer || !pCommandBuffers) return;
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        auto cb = pCommandBuffers[i];
        if (cb) {
            commandBuffer->recorded_commands.insert(
                commandBuffer->recorded_commands.end(),
                cb->recorded_commands.begin(),
                cb->recorded_commands.end()
            );
        }
    }
}

void vkCmdResolveImage(VkCommandBuffer commandBuffer,
                       VkImage srcImage,
                       VkImageLayout srcImageLayout,
                       VkImage dstImage,
                       VkImageLayout dstImageLayout,
                       uint32_t regionCount,
                       const VkImageResolve* pRegions) {
}

void vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer,
                          VkBuffer buffer,
                          VkDeviceSize offset,
                          VkIndexType indexType) {
}

void vkCmdBeginQuery(VkCommandBuffer commandBuffer,
                     VkQueryPool queryPool,
                     uint32_t query,
                     VkQueryControlFlags flags) {
}

void vkCmdEndQuery(VkCommandBuffer commandBuffer,
                   VkQueryPool queryPool,
                   uint32_t query) {
}

void vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer,
                               VkQueryPool queryPool,
                               uint32_t firstQuery,
                               uint32_t queryCount,
                               VkBuffer dstBuffer,
                               VkDeviceSize dstOffset,
                               VkDeviceSize stride,
                               VkQueryResultFlags flags) {
}

static void ApplyPushConstants(VkPipeline pipeline, const uint8_t* push_constants) {
    if (!pipeline || !pipeline->gl_program) return;

    for (const auto& puni : pipeline->push_constant_uniforms) {
        if (puni.location >= 0 && puni.offset + puni.size <= 128) {
            const void* val_ptr = push_constants + puni.offset;
            if (puni.type == 1) { // Float
                if (puni.size == 4 && gl.ProgramUniform1f) {
                    gl.ProgramUniform1f(pipeline->gl_program, puni.location, *(const GLfloat*)val_ptr);
                } else if (puni.size == 16 && gl.ProgramUniform4fv) {
                    gl.ProgramUniform4fv(pipeline->gl_program, puni.location, 1, (const GLfloat*)val_ptr);
                }
            } else if (puni.type == 2) { // UInt
                if (puni.size == 4 && gl.ProgramUniform1ui) {
                    gl.ProgramUniform1ui(pipeline->gl_program, puni.location, *(const GLuint*)val_ptr);
                } else if (puni.size == 16 && gl.ProgramUniform4uiv) {
                    gl.ProgramUniform4uiv(pipeline->gl_program, puni.location, 1, (const GLuint*)val_ptr);
                }
            } else { // Int
                if (puni.size == 4 && gl.ProgramUniform1i) {
                    gl.ProgramUniform1i(pipeline->gl_program, puni.location, *(const GLint*)val_ptr);
                } else if (puni.size == 16 && gl.ProgramUniform4iv) {
                    gl.ProgramUniform4iv(pipeline->gl_program, puni.location, 1, (const GLint*)val_ptr);
                }
            }
        }
    }
}

VkResult vkQueueSubmit(VkQueue queue,
                       uint32_t submitCount,
                       const VkSubmitInfo* pSubmits,
                       VkFence fence) {
    VkPipeline current_pipeline = VK_NULL_HANDLE;
    uint8_t current_push_constants[128] = {0};

    for (uint32_t s = 0; s < submitCount; s++) {
        const auto& submit = pSubmits[s];
        for (uint32_t c = 0; c < submit.commandBufferCount; c++) {
            auto cb = submit.pCommandBuffers[c];
            if (!cb) continue;

            for (const auto& cmd : cb->recorded_commands) {
                switch (cmd.type) {
                    case GLVKCmdType::BindPipeline: {
                        current_pipeline = cmd.bind_pipeline.pipeline;
                        if (current_pipeline && current_pipeline->gl_program) {
                            gl.UseProgram(current_pipeline->gl_program);
                            ApplyPushConstants(current_pipeline, current_push_constants);
                        }
                        break;
                    }
                    case GLVKCmdType::BindDescriptorSets: {
                        for (uint32_t i = 0; i < cmd.bind_descriptor_sets.descriptor_set_count; i++) {
                            auto dset = cmd.bind_descriptor_sets.sets[i];
                            if (!dset) continue;

                            for (const auto& kv : dset->buffer_bindings) {
                                uint32_t binding = kv.first;
                                const auto& binfo = kv.second;
                                if (binfo.gl_buffer != 0) {
                                    gl.BindBufferRange(
                                        GL_SHADER_STORAGE_BUFFER,
                                        binding,
                                        binfo.gl_buffer,
                                        (GLintptr)binfo.offset,
                                        (GLsizeiptr)binfo.range
                                    );
                                }
                            }
                        }
                        break;
                    }
                    case GLVKCmdType::PushConstants: {
                        if (cmd.push_constants.offset + cmd.push_constants.size <= 128) {
                            memcpy(current_push_constants + cmd.push_constants.offset,
                                   cmd.push_constants.data,
                                   cmd.push_constants.size);
                        }

                        if (current_pipeline && current_pipeline->gl_program) {
                            ApplyPushConstants(current_pipeline, current_push_constants);
                        }
                        break;
                    }
                    case GLVKCmdType::PipelineBarrier: {
                        if (gl.MemoryBarrier) {
                            gl.MemoryBarrier(GL_ALL_BARRIER_BITS);
                        }
                        break;
                    }
                    case GLVKCmdType::CopyBuffer: {
                        auto src = cmd.copy_buffer.src_buffer;
                        auto dst = cmd.copy_buffer.dst_buffer;
                        if (src && dst && src->memory && dst->memory) {
                            gl.BindBuffer(GL_COPY_READ_BUFFER, src->memory->gl_buffer);
                            gl.BindBuffer(GL_COPY_WRITE_BUFFER, dst->memory->gl_buffer);
                            gl.CopyBufferSubData(
                                GL_COPY_READ_BUFFER,
                                GL_COPY_WRITE_BUFFER,
                                (GLintptr)(src->memory_offset + cmd.copy_buffer.src_offset),
                                (GLintptr)(dst->memory_offset + cmd.copy_buffer.dst_offset),
                                (GLsizeiptr)cmd.copy_buffer.size
                            );
                            gl.BindBuffer(GL_COPY_READ_BUFFER, 0);
                            gl.BindBuffer(GL_COPY_WRITE_BUFFER, 0);
                        }
                        break;
                    }
                    case GLVKCmdType::FillBuffer: {
                        auto dst = cmd.fill_buffer.dst_buffer;
                        if (dst && dst->memory) {
                            gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, dst->memory->gl_buffer);
                            void* ptr = gl.MapBufferRange(
                                GL_SHADER_STORAGE_BUFFER,
                                (GLintptr)(dst->memory_offset + cmd.fill_buffer.dst_offset),
                                (GLsizeiptr)cmd.fill_buffer.size,
                                GL_MAP_WRITE_BIT
                            );
                            if (ptr) {
                                uint32_t* uptr = (uint32_t*)ptr;
                                size_t count = cmd.fill_buffer.size / sizeof(uint32_t);
                                for (size_t k = 0; k < count; k++) {
                                    uptr[k] = cmd.fill_buffer.data;
                                }
                                gl.UnmapBuffer(GL_SHADER_STORAGE_BUFFER);
                            }
                            gl.BindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
                        }
                        break;
                    }
                    case GLVKCmdType::Dispatch: {
                        if (current_pipeline && current_pipeline->gl_program) {
                            ApplyPushConstants(current_pipeline, current_push_constants);
                        }
                        gl.DispatchCompute(
                            cmd.dispatch.group_count_x,
                            cmd.dispatch.group_count_y,
                            cmd.dispatch.group_count_z
                        );
                        break;
                    }
                    case GLVKCmdType::DispatchIndirect: {
                        if (gl.DispatchComputeIndirect && cmd.dispatch_indirect.buffer && cmd.dispatch_indirect.buffer->memory) {
                            if (current_pipeline && current_pipeline->gl_program) {
                                ApplyPushConstants(current_pipeline, current_push_constants);
                            }
                            gl.BindBuffer(0x90EE /* GL_DISPATCH_INDIRECT_BUFFER */, cmd.dispatch_indirect.buffer->memory->gl_buffer);
                            gl.DispatchComputeIndirect((GLintptr)(cmd.dispatch_indirect.buffer->memory_offset + cmd.dispatch_indirect.offset));
                            gl.BindBuffer(0x90EE, 0);
                        }
                        break;
                    }
                }
            }
        }
    }

    if (gl.MemoryBarrier) {
        gl.MemoryBarrier(GL_ALL_BARRIER_BITS);
    }

    if (fence != VK_NULL_HANDLE) {
        if (fence->sync) {
            gl.DeleteSync(fence->sync);
            fence->sync = nullptr;
        }
        if (gl.FenceSync) {
            fence->sync = gl.FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        }
        fence->signaled = false;
    }

    return VK_SUCCESS;
}

VkResult vkQueueWaitIdle(VkQueue queue) {
    if (gl.MemoryBarrier) {
        gl.MemoryBarrier(GL_ALL_BARRIER_BITS);
    }
    glFinish();
    return VK_SUCCESS;
}

} // extern "C"
