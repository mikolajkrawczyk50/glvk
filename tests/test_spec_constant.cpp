#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

static const uint32_t spec_test_spv[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x00000033, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 
    0x0006000f, 0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x00060010, 0x00000004, 
    0x00000011, 0x00000040, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 
    0x00000004, 0x6e69616d, 0x00000000, 0x00030005, 0x00000008, 0x00006469, 0x00080005, 0x0000000b, 
    0x475f6c67, 0x61626f6c, 0x766e496c, 0x7461636f, 0x496e6f69, 0x00000044, 0x00050005, 0x00000012, 
    0x68737550, 0x736e6f43, 0x00007374, 0x00050006, 0x00000012, 0x00000000, 0x6e756f63, 0x00000074, 
    0x00030005, 0x00000014, 0x00006370, 0x00040005, 0x00000020, 0x7074754f, 0x00427475, 0x00040006, 
    0x00000020, 0x00000000, 0x00000062, 0x00030005, 0x00000022, 0x00000000, 0x00040005, 0x00000025, 
    0x75706e49, 0x00004174, 0x00040006, 0x00000025, 0x00000000, 0x00000061, 0x00030005, 0x00000027, 
    0x00000000, 0x00050005, 0x0000002c, 0x544c554d, 0x494c5049, 0x00005245, 0x00040047, 0x0000000b, 
    0x0000000b, 0x0000001c, 0x00030047, 0x00000012, 0x00000002, 0x00050048, 0x00000012, 0x00000000, 
    0x00000023, 0x00000000, 0x00040047, 0x0000001f, 0x00000006, 0x00000004, 0x00030047, 0x00000020, 
    0x00000003, 0x00040048, 0x00000020, 0x00000000, 0x00000019, 0x00050048, 0x00000020, 0x00000000, 
    0x00000023, 0x00000000, 0x00030047, 0x00000022, 0x00000019, 0x00040047, 0x00000022, 0x00000021, 
    0x00000001, 0x00040047, 0x00000022, 0x00000022, 0x00000000, 0x00040047, 0x00000024, 0x00000006, 
    0x00000004, 0x00030047, 0x00000025, 0x00000003, 0x00040048, 0x00000025, 0x00000000, 0x00000018, 
    0x00050048, 0x00000025, 0x00000000, 0x00000023, 0x00000000, 0x00030047, 0x00000027, 0x00000018, 
    0x00040047, 0x00000027, 0x00000021, 0x00000000, 0x00040047, 0x00000027, 0x00000022, 0x00000000, 
    0x00040047, 0x0000002c, 0x00000001, 0x00000000, 0x00040047, 0x00000032, 0x0000000b, 0x00000019, 
    0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00040015, 0x00000006, 0x00000020, 
    0x00000000, 0x00040020, 0x00000007, 0x00000007, 0x00000006, 0x00040017, 0x00000009, 0x00000006, 
    0x00000003, 0x00040020, 0x0000000a, 0x00000001, 0x00000009, 0x0004003b, 0x0000000a, 0x0000000b, 
    0x00000001, 0x0004002b, 0x00000006, 0x0000000c, 0x00000000, 0x00040020, 0x0000000d, 0x00000001, 
    0x00000006, 0x00040015, 0x00000011, 0x00000020, 0x00000001, 0x0003001e, 0x00000012, 0x00000011, 
    0x00040020, 0x00000013, 0x00000009, 0x00000012, 0x0004003b, 0x00000013, 0x00000014, 0x00000009, 
    0x0004002b, 0x00000011, 0x00000015, 0x00000000, 0x00040020, 0x00000016, 0x00000009, 0x00000011, 
    0x00020014, 0x0000001a, 0x00030016, 0x0000001e, 0x00000020, 0x0003001d, 0x0000001f, 0x0000001e, 
    0x0003001e, 0x00000020, 0x0000001f, 0x00040020, 0x00000021, 0x00000002, 0x00000020, 0x0004003b, 
    0x00000021, 0x00000022, 0x00000002, 0x0003001d, 0x00000024, 0x0000001e, 0x0003001e, 0x00000025, 
    0x00000024, 0x00040020, 0x00000026, 0x00000002, 0x00000025, 0x0004003b, 0x00000026, 0x00000027, 
    0x00000002, 0x00040020, 0x00000029, 0x00000002, 0x0000001e, 0x00040032, 0x00000011, 0x0000002c, 
    0x00000001, 0x0004002b, 0x00000006, 0x00000030, 0x00000040, 0x0004002b, 0x00000006, 0x00000031, 
    0x00000001, 0x0006002c, 0x00000009, 0x00000032, 0x00000030, 0x00000031, 0x00000031, 0x00050036, 
    0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003b, 0x00000007, 
    0x00000008, 0x00000007, 0x00050041, 0x0000000d, 0x0000000e, 0x0000000b, 0x0000000c, 0x0004003d, 
    0x00000006, 0x0000000f, 0x0000000e, 0x0003003e, 0x00000008, 0x0000000f, 0x0004003d, 0x00000006, 
    0x00000010, 0x00000008, 0x00050041, 0x00000016, 0x00000017, 0x00000014, 0x00000015, 0x0004003d, 
    0x00000011, 0x00000018, 0x00000017, 0x0004007c, 0x00000006, 0x00000019, 0x00000018, 0x000500b0, 
    0x0000001a, 0x0000001b, 0x00000010, 0x00000019, 0x000300f7, 0x0000001d, 0x00000000, 0x000400fa, 
    0x0000001b, 0x0000001c, 0x0000001d, 0x000200f8, 0x0000001c, 0x0004003d, 0x00000006, 0x00000023, 
    0x00000008, 0x0004003d, 0x00000006, 0x00000028, 0x00000008, 0x00060041, 0x00000029, 0x0000002a, 
    0x00000027, 0x00000015, 0x00000028, 0x0004003d, 0x0000001e, 0x0000002b, 0x0000002a, 0x0004006f, 
    0x0000001e, 0x0000002d, 0x0000002c, 0x00050085, 0x0000001e, 0x0000002e, 0x0000002b, 0x0000002d, 
    0x00060041, 0x00000029, 0x0000002f, 0x00000022, 0x00000015, 0x00000023, 0x0003003e, 0x0000002f, 
    0x0000002e, 0x000200f9, 0x0000001d, 0x000200f8, 0x0000001d, 0x000100fd, 0x00010038
};

int main() {
    std::cout << "[TEST] Starting GLVK Specialization Constant Test..." << std::endl;

    VkInstanceCreateInfo inst_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkInstance instance = VK_NULL_HANDLE;
    VkResult res = vkCreateInstance(&inst_info, nullptr, &instance);
    assert(res == VK_SUCCESS);

    uint32_t phys_count = 1;
    VkPhysicalDevice phys_device;
    vkEnumeratePhysicalDevices(instance, &phys_count, &phys_device);

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo dev_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    dev_info.queueCreateInfoCount = 1;
    dev_info.pQueueCreateInfos = &queue_info;

    VkDevice device = VK_NULL_HANDLE;
    vkCreateDevice(phys_device, &dev_info, nullptr, &device);

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, 0, 0, &queue);

    const int count = 128;
    const size_t buffer_size = count * sizeof(float);

    VkBufferCreateInfo buf_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buf_info.size = buffer_size;
    buf_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VkBuffer buf_a, buf_b;
    vkCreateBuffer(device, &buf_info, nullptr, &buf_a);
    vkCreateBuffer(device, &buf_info, nullptr, &buf_b);

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, buf_a, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = 0;

    VkDeviceMemory mem_a, mem_b;
    vkAllocateMemory(device, &alloc_info, nullptr, &mem_a);
    vkAllocateMemory(device, &alloc_info, nullptr, &mem_b);

    vkBindBufferMemory(device, buf_a, mem_a, 0);
    vkBindBufferMemory(device, buf_b, mem_b, 0);

    float* ptr_a = nullptr;
    float* ptr_b = nullptr;
    vkMapMemory(device, mem_a, 0, buffer_size, 0, (void**)&ptr_a);
    vkMapMemory(device, mem_b, 0, buffer_size, 0, (void**)&ptr_b);

    for (int i = 0; i < count; i++) {
        ptr_a[i] = (float)(i + 1);
        ptr_b[i] = 0.0f;
    }

    vkUnmapMemory(device, mem_a);
    vkUnmapMemory(device, mem_b);

    VkShaderModuleCreateInfo shader_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    shader_info.codeSize = sizeof(spec_test_spv);
    shader_info.pCode = spec_test_spv;

    VkShaderModule shader_module = VK_NULL_HANDLE;
    vkCreateShaderModule(device, &shader_info, nullptr, &shader_module);

    VkDescriptorSetLayoutBinding bindings[2] = {};
    bindings[0] = { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
    bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };

    VkDescriptorSetLayoutCreateInfo dsl_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    dsl_info.bindingCount = 2;
    dsl_info.pBindings = bindings;

    VkDescriptorSetLayout desc_layout = VK_NULL_HANDLE;
    vkCreateDescriptorSetLayout(device, &dsl_info, nullptr, &desc_layout);

    VkPushConstantRange pc_range = { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int) };

    VkPipelineLayoutCreateInfo pipe_layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipe_layout_info.setLayoutCount = 1;
    pipe_layout_info.pSetLayouts = &desc_layout;
    pipe_layout_info.pushConstantRangeCount = 1;
    pipe_layout_info.pPushConstantRanges = &pc_range;

    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    vkCreatePipelineLayout(device, &pipe_layout_info, nullptr, &pipeline_layout);

    // Specialization constant: MULTIPLIER (constant_id = 0) = 7
    int spec_val = 7;
    VkSpecializationMapEntry spec_entry = { 0, 0, sizeof(int) };
    VkSpecializationInfo spec_info = {};
    spec_info.mapEntryCount = 1;
    spec_info.pMapEntries = &spec_entry;
    spec_info.dataSize = sizeof(int);
    spec_info.pData = &spec_val;

    VkComputePipelineCreateInfo pipe_info = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipe_info.layout = pipeline_layout;
    pipe_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipe_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipe_info.stage.module = shader_module;
    pipe_info.stage.pName = "main";
    pipe_info.stage.pSpecializationInfo = &spec_info;

    VkPipeline pipeline = VK_NULL_HANDLE;
    res = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipe_info, nullptr, &pipeline);
    assert(res == VK_SUCCESS && pipeline != VK_NULL_HANDLE);
    std::cout << "[TEST] Pipeline with Specialization Constant (MULTIPLIER = 7) created." << std::endl;

    VkDescriptorPoolSize pool_size = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 };
    VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &desc_pool);

    VkDescriptorSetAllocateInfo dalloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    dalloc.descriptorPool = desc_pool;
    dalloc.descriptorSetCount = 1;
    dalloc.pSetLayouts = &desc_layout;

    VkDescriptorSet desc_set = VK_NULL_HANDLE;
    vkAllocateDescriptorSets(device, &dalloc, &desc_set);

    VkDescriptorBufferInfo buf_infos[2] = {
        { buf_a, 0, VK_WHOLE_SIZE },
        { buf_b, 0, VK_WHOLE_SIZE }
    };

    VkWriteDescriptorSet writes[2] = {};
    for (int i = 0; i < 2; i++) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = desc_set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &buf_infos[i];
    }
    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);

    VkCommandPoolCreateInfo cmd_pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    vkCreateCommandPool(device, &cmd_pool_info, nullptr, &cmd_pool);

    VkCommandBufferAllocateInfo cb_alloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cb_alloc.commandPool = cmd_pool;
    cb_alloc.commandBufferCount = 1;
    cb_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device, &cb_alloc, &cmd_buf);

    VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(cmd_buf, &begin_info);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &desc_set, 0, nullptr);

    int push_count = count;
    vkCmdPushConstants(cmd_buf, pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &push_count);
    vkCmdDispatch(cmd_buf, (count + 63) / 64, 1, 1);
    vkEndCommandBuffer(cmd_buf);

    VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device, &fence_info, nullptr, &fence);

    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf;

    vkQueueSubmit(queue, 1, &submit_info, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    // Verify b[i] == a[i] * 7.0f
    vkMapMemory(device, mem_b, 0, buffer_size, 0, (void**)&ptr_b);
    int errors = 0;
    for (int i = 0; i < count; i++) {
        float expected = (float)(i + 1) * 7.0f;
        if (std::fabs(ptr_b[i] - expected) > 1e-4f) {
            std::cerr << "[ERROR] Mismatch at index " << i << ": got " << ptr_b[i] << ", expected " << expected << std::endl;
            errors++;
            if (errors > 5) break;
        }
    }
    vkUnmapMemory(device, mem_b);

    if (errors == 0) {
        std::cout << "[SUCCESS] Specialization constant MULTIPLIER = 7 verified across all elements!" << std::endl;
    } else {
        std::cerr << "[FAILURE] Specialization constant test failed." << std::endl;
        return 1;
    }

    // Cleanup
    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buf);
    vkDestroyCommandPool(device, cmd_pool, nullptr);
    vkDestroyDescriptorPool(device, desc_pool, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(device, desc_layout, nullptr);
    vkDestroyShaderModule(device, shader_module, nullptr);
    vkDestroyBuffer(device, buf_a, nullptr);
    vkDestroyBuffer(device, buf_b, nullptr);
    vkFreeMemory(device, mem_a, nullptr);
    vkFreeMemory(device, mem_b, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    std::cout << "[TEST] Cleaned up and exited cleanly." << std::endl;
    return 0;
}
