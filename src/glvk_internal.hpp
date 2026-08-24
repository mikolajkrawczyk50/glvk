#pragma once

#include <vulkan/vulkan.h>
#include "gl_backend.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>

struct VkInstance_T {
    std::vector<VkPhysicalDevice> physical_devices;
};

struct VkPhysicalDevice_T {
    VkInstance instance = VK_NULL_HANDLE;
    GLGPUInfo gpu_info;
};

struct VkDevice_T {
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkQueue default_queue = VK_NULL_HANDLE;
};

struct VkQueue_T {
    VkDevice device = VK_NULL_HANDLE;
    uint32_t family_index = 0;
    uint32_t queue_index = 0;
};

struct VkDeviceMemory_T {
    GLuint gl_buffer = 0;
    VkDeviceSize size = 0;
    uint32_t memory_type_index = 0;
    VkMemoryPropertyFlags property_flags = 0;
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

struct VkDescriptorSetLayoutBindingInfo {
    uint32_t binding;
    VkDescriptorType descriptorType;
    uint32_t descriptorCount;
    VkShaderStageFlags stageFlags;
};

struct VkDescriptorSetLayout_T {
    std::vector<VkDescriptorSetLayoutBindingInfo> bindings;
};

struct VkPipelineLayout_T {
    std::vector<VkDescriptorSetLayout> set_layouts;
    std::vector<VkPushConstantRange> push_constant_ranges;
};

struct PushConstantUniformMap {
    GLint location = -1;
    uint32_t offset = 0;
    uint32_t size = 0;
};

struct VkPipeline_T {
    GLuint gl_program = 0;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    std::vector<PushConstantUniformMap> push_constant_uniforms;
};

struct BoundBufferInfo {
    GLuint gl_buffer = 0;
    VkDeviceSize offset = 0;
    VkDeviceSize range = 0;
};

struct BoundImageInfo {
    GLuint gl_texture = 0;
    GLenum gl_format = 0;
};

struct VkDescriptorSet_T {
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    // Map binding index -> bound buffer info
    std::unordered_map<uint32_t, BoundBufferInfo> buffer_bindings;
    std::unordered_map<uint32_t, BoundImageInfo> image_bindings;
};

struct VkDescriptorPool_T {
    std::vector<VkDescriptorSet> allocated_sets;
};

enum class GLVKCmdType {
    BindPipeline,
    BindDescriptorSets,
    PushConstants,
    PipelineBarrier,
    CopyBuffer,
    FillBuffer,
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
            uint8_t data[128]; // Max standard push constant size
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

struct VkCommandPool_T {
    uint32_t queue_family_index = 0;
    VkCommandPoolCreateFlags flags = 0;
};

struct VkCommandBuffer_T {
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    std::vector<GLVKCmd> recorded_commands;
    bool is_recording = false;
};

struct VkFence_T {
    GLsync sync = nullptr;
    bool signaled = false;
};

struct VkSemaphore_T {
    bool dummy = true;
};

struct VkPipelineCache_T {
    std::vector<uint8_t> data;
};

struct VkQueryPool_T {
    uint32_t count = 0;
};
