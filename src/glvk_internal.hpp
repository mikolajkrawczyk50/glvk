#pragma once

#include <vulkan/vulkan.h>
#include "gl_loader.hpp"
#include "gl_backend.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>

inline int GetGLVKLogLevel() {
    static int level = []() {
        const char* env = getenv("GLVK_DEBUG");
        if (!env) env = getenv("GLVK_LOG");
        if (!env) env = getenv("GLVK_LOG_LEVEL");
        if (env) {
            if (strcmp(env, "1") == 0 || strcasecmp(env, "true") == 0 || strcasecmp(env, "info") == 0) return 1;
            if (strcmp(env, "2") == 0 || strcasecmp(env, "debug") == 0) return 2;
            if (strcmp(env, "3") == 0 || strcasecmp(env, "trace") == 0) return 3;
            if (strcmp(env, "0") == 0 || strcasecmp(env, "false") == 0) return 0;
            return atoi(env);
        }
        return 0;
    }();
    return level;
}

struct GLVKPushConstantUniform {
    GLint location;
    uint32_t offset;
    uint32_t size;
    uint32_t type; // 0 = int, 1 = float, 2 = uint
};

struct VkInstance_T {
    std::vector<struct VkPhysicalDevice_T*> physical_devices;
};

struct VkPhysicalDevice_T {
    VkInstance instance = nullptr;
    GLGPUInfo gpu_info;
};

struct VkQueue_T {
    VkDevice device = nullptr;
    uint32_t family_index = 0;
    uint32_t queue_index = 0;
};

struct VkDevice_T {
    VkPhysicalDevice physical_device = nullptr;
    VkQueue default_queue = nullptr;
};

struct VkDeviceMemory_T {
    VkDeviceSize size = 0;
    uint32_t memory_type_index = 0;
    GLuint gl_buffer = 0;
    void* mapped_ptr = nullptr;
    VkDeviceSize mapped_offset = 0;
    VkDeviceSize mapped_size = 0;
};

struct VkBuffer_T {
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = 0;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize memory_offset = 0;
};

struct VkShaderModule_T {
    std::vector<uint32_t> spirv_words;
};

struct VkPipelineLayout_T {
    std::vector<VkDescriptorSetLayout> set_layouts;
    std::vector<VkPushConstantRange> push_constant_ranges;
};

struct VkPipeline_T {
    VkPipelineLayout layout = VK_NULL_HANDLE;
    GLuint gl_program = 0;
    std::vector<GLVKPushConstantUniform> push_constant_uniforms;
};

struct VkPipelineCache_T {
    uint32_t dummy = 0;
};

struct VkQueryPool_T {
    uint32_t count = 0;
};

struct GLVKBufferBindingInfo {
    GLuint gl_buffer = 0;
    VkDeviceSize offset = 0;
    VkDeviceSize range = 0;
    uint32_t set_index = 0; // Vulkan descriptor set index for flat binding calculation
};

struct VkDescriptorSetLayout_T {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

struct VkDescriptorPool_T {
    uint32_t max_sets = 0;
    std::vector<VkDescriptorSet> allocated_sets;
};

struct VkDescriptorSet_T {
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    std::unordered_map<uint32_t, GLVKBufferBindingInfo> buffer_bindings;
};

struct VkFence_T {
    GLsync sync = nullptr;
    bool signaled = false;
};

struct VkSemaphore_T {
    bool signaled = false;
};

enum class GLVKCmdType {
    BindPipeline,
    BindDescriptorSets,
    PushConstants,
    PipelineBarrier,
    CopyBuffer,
    FillBuffer,
    UpdateBuffer,
    Dispatch,
    DispatchIndirect
};

struct GLVKCmd {
    GLVKCmdType type;
    union {
        struct {
            VkPipeline pipeline;
        } bind_pipeline;

        struct {
            uint32_t first_set;
            uint32_t descriptor_set_count;
            VkDescriptorSet sets[8];
        } bind_descriptor_sets;

        struct {
            uint32_t offset;
            uint32_t size;
            uint8_t data[128];
        } push_constants;

        struct {
            VkPipelineStageFlags src_stage_mask;
            VkPipelineStageFlags dst_stage_mask;
        } pipeline_barrier;

        struct {
            VkBuffer src_buffer;
            VkBuffer dst_buffer;
            VkDeviceSize src_offset;
            VkDeviceSize dst_offset;
            VkDeviceSize size;
        } copy_buffer;

        struct {
            VkBuffer dst_buffer;
            VkDeviceSize dst_offset;
            VkDeviceSize size;
            uint32_t data;
        } fill_buffer;

        struct {
            VkBuffer dst_buffer;
            VkDeviceSize dst_offset;
            VkDeviceSize data_size;
            void* data_copy; // dynamically allocated, freed after execution
        } update_buffer;

        struct {
            uint32_t group_count_x;
            uint32_t group_count_y;
            uint32_t group_count_z;
        } dispatch;

        struct {
            VkBuffer buffer;
            VkDeviceSize offset;
        } dispatch_indirect;
    };
};

struct VkCommandBuffer_T {
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBufferLevel level;
    bool is_recording = false;
    std::vector<GLVKCmd> recorded_commands;
};

struct VkCommandPool_T {
    uint32_t queue_family_index = 0;
    VkCommandPoolCreateFlags flags = 0;
};
