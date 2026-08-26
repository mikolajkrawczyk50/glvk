#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <cassert>

#define VK_CHECK(x) do { \
    VkResult err = (x); \
    if (err != VK_SUCCESS) { \
        std::cerr << "[FAIL] " << #x << " returned " << err << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    } \
} while(0)

// Valid SPIR-V compiled from:
// #version 450
// layout(local_size_x = 64) in;
// layout(push_constant) uniform PushConsts { int count; } pc;
// layout(std430, binding = 0) readonly buffer InputA { float a[]; };
// layout(std430, binding = 1) readonly buffer InputB { float b[]; };
// layout(std430, binding = 2) writeonly buffer OutputC { float c[]; };
// void main() {
//     uint id = gl_GlobalInvocationID.x;
//     if (int(id) < pc.count) {
//         c[id] = a[id] + b[id];
//     }
// }
static const uint32_t vecadd_spv[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x00000038, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 
    0x0006000f, 0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x00060010, 0x00000004, 
    0x00000011, 0x00000040, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 
    0x00000004, 0x6e69616d, 0x00000000, 0x00030005, 0x00000008, 0x00006469, 0x00080005, 0x0000000b, 
    0x475f6c67, 0x61626f6c, 0x766e496c, 0x7461636f, 0x496e6f69, 0x00000044, 0x00050005, 0x00000012, 
    0x68737550, 0x736e6f43, 0x00007374, 0x00050006, 0x00000012, 0x00000000, 0x6e756f63, 0x00000074, 
    0x00030005, 0x00000014, 0x00006370, 0x00040005, 0x00000020, 0x7074754f, 0x00437475, 0x00040006, 
    0x00000020, 0x00000000, 0x00000063, 0x00030005, 0x00000022, 0x00000000, 0x00040005, 0x00000025, 
    0x75706e49, 0x00417441, 0x00040006, 0x00000025, 0x00000000, 0x00000061, 0x00030005, 0x00000027, 
    0x00000000, 0x00040005, 0x0000002d, 0x75706e49, 0x00427442, 0x00040006, 0x0000002d, 0x00000000, 
    0x00000062, 0x00030005, 0x0000002f, 0x00000000, 0x00040047, 0x0000000b, 0x0000000b, 0x0000001c, 
    0x00030047, 0x00000012, 0x00000002, 0x00050048, 0x00000012, 0x00000000, 0x00000023, 0x00000000, 
    0x00040047, 0x0000001f, 0x00000006, 0x00000004, 0x00030047, 0x00000020, 0x00000003, 0x00040048, 
    0x00000020, 0x00000000, 0x00000019, 0x00050048, 0x00000020, 0x00000000, 0x00000023, 0x00000000, 
    0x00030047, 0x00000022, 0x00000019, 0x00040047, 0x00000022, 0x00000021, 0x00000002, 0x00040047, 
    0x00000022, 0x00000022, 0x00000000, 0x00040047, 0x00000024, 0x00000006, 0x00000004, 0x00030047, 
    0x00000025, 0x00000003, 0x00040048, 0x00000025, 0x00000000, 0x00000018, 0x00050048, 0x00000025, 
    0x00000000, 0x00000023, 0x00000000, 0x00030047, 0x00000027, 0x00000018, 0x00040047, 0x00000027, 
    0x00000021, 0x00000000, 0x00040047, 0x00000027, 0x00000022, 0x00000000, 0x00040047, 0x0000002c, 
    0x00000006, 0x00000004, 0x00030047, 0x0000002d, 0x00000003, 0x00040048, 0x0000002d, 0x00000000, 
    0x00000018, 0x00050048, 0x0000002d, 0x00000000, 0x00000023, 0x00000000, 0x00030047, 0x0000002f, 
    0x00000018, 0x00040047, 0x0000002f, 0x00000021, 0x00000001, 0x00040047, 0x0000002f, 0x00000022, 
    0x00000000, 0x00040047, 0x00000037, 0x0000000b, 0x00000019, 0x00020013, 0x00000002, 0x00030021, 
    0x00000003, 0x00000002, 0x00040015, 0x00000006, 0x00000020, 0x00000000, 0x00040020, 0x00000007, 
    0x00000007, 0x00000006, 0x00040017, 0x00000009, 0x00000006, 0x00000003, 0x00040020, 0x0000000a, 
    0x00000001, 0x00000009, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000001, 0x0004002b, 0x00000006, 
    0x0000000c, 0x00000000, 0x00040020, 0x0000000d, 0x00000001, 0x00000006, 0x00040015, 0x00000011, 
    0x00000020, 0x00000001, 0x0003001e, 0x00000012, 0x00000011, 0x00040020, 0x00000013, 0x00000009, 
    0x00000012, 0x0004003b, 0x00000013, 0x00000014, 0x00000009, 0x0004002b, 0x00000011, 0x00000015, 
    0x00000000, 0x00040020, 0x00000016, 0x00000009, 0x00000011, 0x00020014, 0x0000001a, 0x00030016, 
    0x0000001e, 0x00000020, 0x0003001d, 0x0000001f, 0x0000001e, 0x0003001e, 0x00000020, 0x0000001f, 
    0x00040020, 0x00000021, 0x00000002, 0x00000020, 0x0004003b, 0x00000021, 0x00000022, 0x00000002, 
    0x0003001d, 0x00000024, 0x0000001e, 0x0003001e, 0x00000025, 0x00000024, 0x00040020, 0x00000026, 
    0x00000002, 0x00000025, 0x0004003b, 0x00000026, 0x00000027, 0x00000002, 0x00040020, 0x00000029, 
    0x00000002, 0x0000001e, 0x0003001d, 0x0000002c, 0x0000001e, 0x0003001e, 0x0000002d, 0x0000002c, 
    0x00040020, 0x0000002e, 0x00000002, 0x0000002d, 0x0004003b, 0x0000002e, 0x0000002f, 0x00000002, 
    0x0004002b, 0x00000006, 0x00000035, 0x00000040, 0x0004002b, 0x00000006, 0x00000036, 0x00000001, 
    0x0006002c, 0x00000009, 0x00000037, 0x00000035, 0x00000036, 0x00000036, 0x00050036, 0x00000002, 
    0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003b, 0x00000007, 0x00000008, 
    0x00000007, 0x00050041, 0x0000000d, 0x0000000e, 0x0000000b, 0x0000000c, 0x0004003d, 0x00000006, 
    0x0000000f, 0x0000000e, 0x0003003e, 0x00000008, 0x0000000f, 0x0004003d, 0x00000006, 0x00000010, 
    0x00000008, 0x00050041, 0x00000016, 0x00000017, 0x00000014, 0x00000015, 0x0004003d, 0x00000011, 
    0x00000018, 0x00000017, 0x0004007c, 0x00000006, 0x00000019, 0x00000018, 0x000500b0, 0x0000001a, 
    0x0000001b, 0x00000010, 0x00000019, 0x000300f7, 0x0000001d, 0x00000000, 0x000400fa, 0x0000001b, 
    0x0000001c, 0x0000001d, 0x000200f8, 0x0000001c, 0x0004003d, 0x00000006, 0x00000023, 0x00000008, 
    0x0004003d, 0x00000006, 0x00000028, 0x00000008, 0x00060041, 0x00000029, 0x0000002a, 0x00000027, 
    0x00000015, 0x00000028, 0x0004003d, 0x0000001e, 0x0000002b, 0x0000002a, 0x0004003d, 0x00000006, 
    0x00000030, 0x00000008, 0x00060041, 0x00000029, 0x00000031, 0x0000002f, 0x00000015, 0x00000030, 
    0x0004003d, 0x0000001e, 0x00000032, 0x00000031, 0x00050081, 0x0000001e, 0x00000033, 0x0000002b, 
    0x00000032, 0x00060041, 0x00000029, 0x00000034, 0x00000022, 0x00000015, 0x00000023, 0x0003003e, 
    0x00000034, 0x00000033, 0x000200f9, 0x0000001d, 0x000200f8, 0x0000001d, 0x000100fd, 0x00010038
};

bool test_vulkan_full_suite() {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "  GLVK Comprehensive Vulkan API Test Suite" << std::endl;
    std::cout << "==========================================" << std::endl;

    // 1. Instance & Physical Device
    VkInstanceCreateInfo inst_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkInstance instance = VK_NULL_HANDLE;
    VK_CHECK(vkCreateInstance(&inst_info, nullptr, &instance));

    uint32_t phys_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &phys_count, nullptr));
    assert(phys_count > 0);

    std::vector<VkPhysicalDevice> phys_devices(phys_count);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &phys_count, phys_devices.data()));
    VkPhysicalDevice phys_dev = phys_devices[0];

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(phys_dev, &props);
    std::cout << "[INFO] Device: " << props.deviceName << " (Driver v" << props.driverVersion << ")" << std::endl;

    // 2. Logical Device & Queue
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo dev_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    dev_info.queueCreateInfoCount = 1;
    dev_info.pQueueCreateInfos = &queue_info;

    VkDevice device = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDevice(phys_dev, &dev_info, nullptr, &device));

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, 0, 0, &queue);

    // 3. Command Pool & Command Buffer
    VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.queueFamilyIndex = 0;
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateCommandPool(device, &pool_info, nullptr, &cmd_pool));

    VkCommandBufferAllocateInfo cb_alloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cb_alloc.commandPool = cmd_pool;
    cb_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_alloc.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(device, &cb_alloc, &cmd));

    // 4. Memory Allocations & Buffer Management
    const int N = 256;
    const size_t buf_size = N * sizeof(float);

    VkBufferCreateInfo buf_create = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buf_create.size = buf_size;
    buf_create.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkBuffer buf_a, buf_b, buf_out, buf_copy;
    VK_CHECK(vkCreateBuffer(device, &buf_create, nullptr, &buf_a));
    VK_CHECK(vkCreateBuffer(device, &buf_create, nullptr, &buf_b));
    VK_CHECK(vkCreateBuffer(device, &buf_create, nullptr, &buf_out));
    VK_CHECK(vkCreateBuffer(device, &buf_create, nullptr, &buf_copy));

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(device, buf_a, &mem_req);

    VkMemoryAllocateInfo mem_alloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    mem_alloc.allocationSize = mem_req.size;
    mem_alloc.memoryTypeIndex = 0;

    VkDeviceMemory mem_a, mem_b, mem_out, mem_copy;
    VK_CHECK(vkAllocateMemory(device, &mem_alloc, nullptr, &mem_a));
    VK_CHECK(vkAllocateMemory(device, &mem_alloc, nullptr, &mem_b));
    VK_CHECK(vkAllocateMemory(device, &mem_alloc, nullptr, &mem_out));
    VK_CHECK(vkAllocateMemory(device, &mem_alloc, nullptr, &mem_copy));

    VK_CHECK(vkBindBufferMemory(device, buf_a, mem_a, 0));
    VK_CHECK(vkBindBufferMemory(device, buf_b, mem_b, 0));
    VK_CHECK(vkBindBufferMemory(device, buf_out, mem_out, 0));
    VK_CHECK(vkBindBufferMemory(device, buf_copy, mem_copy, 0));

    // Populate buffers via vkMapMemory
    float* ptr_a = nullptr;
    float* ptr_b = nullptr;
    VK_CHECK(vkMapMemory(device, mem_a, 0, buf_size, 0, (void**)&ptr_a));
    VK_CHECK(vkMapMemory(device, mem_b, 0, buf_size, 0, (void**)&ptr_b));
    for (int i = 0; i < N; i++) {
        ptr_a[i] = (float)i * 1.5f;
        ptr_b[i] = (float)(N - i) * 0.5f;
    }
    vkUnmapMemory(device, mem_a);
    vkUnmapMemory(device, mem_b);

    // 5. Shader Module
    VkShaderModuleCreateInfo sm_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    sm_info.codeSize = sizeof(vecadd_spv);
    sm_info.pCode = vecadd_spv;
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VK_CHECK(vkCreateShaderModule(device, &sm_info, nullptr, &shader_module));

    // 6. Descriptor Set Layout & Pipeline Layout
    VkDescriptorSetLayoutBinding bindings[3] = {};
    for (uint32_t i = 0; i < 3; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo dsl_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    dsl_info.bindingCount = 3;
    dsl_info.pBindings = bindings;
    VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &dsl_info, nullptr, &set_layout));

    VkPushConstantRange pc_range = {};
    pc_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pc_range.offset = 0;
    pc_range.size = sizeof(int);

    VkPipelineLayoutCreateInfo pl_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pl_info.setLayoutCount = 1;
    pl_info.pSetLayouts = &set_layout;
    pl_info.pushConstantRangeCount = 1;
    pl_info.pPushConstantRanges = &pc_range;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VK_CHECK(vkCreatePipelineLayout(device, &pl_info, nullptr, &pipeline_layout));

    // 7. Compute Pipeline
    VkComputePipelineCreateInfo cp_info = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    cp_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cp_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    cp_info.stage.module = shader_module;
    cp_info.stage.pName = "main";
    cp_info.layout = pipeline_layout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &cp_info, nullptr, &pipeline));

    // 8. Descriptor Pool & Descriptor Set
    VkDescriptorPoolSize pool_size = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 };
    VkDescriptorPoolCreateInfo dp_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    dp_info.maxSets = 1;
    dp_info.poolSizeCount = 1;
    dp_info.pPoolSizes = &pool_size;
    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorPool(device, &dp_info, nullptr, &desc_pool));

    VkDescriptorSetAllocateInfo ds_alloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    ds_alloc.descriptorPool = desc_pool;
    ds_alloc.descriptorSetCount = 1;
    ds_alloc.pSetLayouts = &set_layout;
    VkDescriptorSet desc_set = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateDescriptorSets(device, &ds_alloc, &desc_set));

    VkDescriptorBufferInfo buf_infos[3] = {
        { buf_a, 0, buf_size },
        { buf_b, 0, buf_size },
        { buf_out, 0, buf_size }
    };

    VkWriteDescriptorSet writes[3] = {};
    for (uint32_t i = 0; i < 3; i++) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = desc_set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &buf_infos[i];
    }
    vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);

    // 9. Record Commands: Compute Dispatch + Buffer Copy + FillBuffer + UpdateBuffer
    VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &desc_set, 0, nullptr);

    int count_val = N;
    vkCmdPushConstants(cmd, pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &count_val);

    uint32_t group_count_x = (N + 63) / 64;
    vkCmdDispatch(cmd, group_count_x, 1, 1);

    // Barrier between dispatch and copy
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

    // Copy buf_out -> buf_copy
    VkBufferCopy copy_region = { 0, 0, buf_size };
    vkCmdCopyBuffer(cmd, buf_out, buf_copy, 1, &copy_region);

    VK_CHECK(vkEndCommandBuffer(cmd));

    // 10. Fence Synchronization & Queue Submit
    VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence = VK_NULL_HANDLE;
    VK_CHECK(vkCreateFence(device, &fence_info, nullptr, &fence));

    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    VK_CHECK(vkQueueSubmit(queue, 1, &submit, fence));

    VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkGetFenceStatus(device, fence));

    // 11. Verify Compute & Copy Output
    float* out_ptr = nullptr;
    float* copy_ptr = nullptr;
    VK_CHECK(vkMapMemory(device, mem_out, 0, buf_size, 0, (void**)&out_ptr));
    VK_CHECK(vkMapMemory(device, mem_copy, 0, buf_size, 0, (void**)&copy_ptr));

    bool compute_pass = true;
    for (int i = 0; i < N; i++) {
        float expected = (float)i * 1.5f + (float)(N - i) * 0.5f;
        if (std::abs(out_ptr[i] - expected) > 1e-4f) {
            std::cerr << "[FAIL] Compute Mismatch at " << i << ": got " << out_ptr[i] << ", expected " << expected << std::endl;
            compute_pass = false;
            break;
        }
        if (std::abs(copy_ptr[i] - expected) > 1e-4f) {
            std::cerr << "[FAIL] CopyBuffer Mismatch at " << i << ": got " << copy_ptr[i] << ", expected " << expected << std::endl;
            compute_pass = false;
            break;
        }
    }
    vkUnmapMemory(device, mem_out);
    vkUnmapMemory(device, mem_copy);

    if (compute_pass) {
        std::cout << "[PASS] Compute dispatch + PushConstants + CopyBuffer + Fence sync verified successfully!" << std::endl;
    }

    // 12. Test FillBuffer & UpdateBuffer
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

    // Fill buffer with bit pattern 0x3F800000 (float 1.0f)
    uint32_t fill_val = 0x3F800000;
    vkCmdFillBuffer(cmd, buf_copy, 0, buf_size, fill_val);

    // Update first 4 floats with custom data
    float update_vals[4] = { 42.0f, 43.0f, 44.0f, 45.0f };
    vkCmdUpdateBuffer(cmd, buf_copy, 0, sizeof(update_vals), update_vals);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VK_CHECK(vkResetFences(device, 1, &fence));
    VK_CHECK(vkQueueSubmit(queue, 1, &submit, fence));
    VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

    VK_CHECK(vkMapMemory(device, mem_copy, 0, buf_size, 0, (void**)&copy_ptr));
    bool fill_update_pass = true;
    for (int i = 0; i < 4; i++) {
        if (std::abs(copy_ptr[i] - update_vals[i]) > 1e-5f) {
            std::cerr << "[FAIL] UpdateBuffer mismatch at " << i << ": got " << copy_ptr[i] << ", expected " << update_vals[i] << std::endl;
            fill_update_pass = false;
        }
    }
    for (int i = 4; i < N; i++) {
        if (std::abs(copy_ptr[i] - 1.0f) > 1e-5f) {
            std::cerr << "[FAIL] FillBuffer mismatch at " << i << ": got " << copy_ptr[i] << ", expected 1.0" << std::endl;
            fill_update_pass = false;
        }
    }
    vkUnmapMemory(device, mem_copy);

    if (fill_update_pass) {
        std::cout << "[PASS] vkCmdFillBuffer & vkCmdUpdateBuffer verified successfully!" << std::endl;
    }

    // 13. Test Indirect Dispatch
    VkDispatchIndirectCommand ind_cmd = { (uint32_t)group_count_x, 1, 1 };
    VkBufferCreateInfo ind_buf_create = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    ind_buf_create.size = sizeof(VkDispatchIndirectCommand);
    ind_buf_create.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VkBuffer buf_ind = VK_NULL_HANDLE;
    VK_CHECK(vkCreateBuffer(device, &ind_buf_create, nullptr, &buf_ind));

    VkDeviceMemory mem_ind = VK_NULL_HANDLE;
    VkMemoryAllocateInfo ind_alloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ind_alloc.allocationSize = sizeof(VkDispatchIndirectCommand);
    ind_alloc.memoryTypeIndex = 0;
    VK_CHECK(vkAllocateMemory(device, &ind_alloc, nullptr, &mem_ind));
    VK_CHECK(vkBindBufferMemory(device, buf_ind, mem_ind, 0));

    void* ind_ptr = nullptr;
    VK_CHECK(vkMapMemory(device, mem_ind, 0, sizeof(VkDispatchIndirectCommand), 0, &ind_ptr));
    memcpy(ind_ptr, &ind_cmd, sizeof(VkDispatchIndirectCommand));
    vkUnmapMemory(device, mem_ind);

    // Clear buf_out first
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));
    vkCmdFillBuffer(cmd, buf_out, 0, buf_size, 0);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &desc_set, 0, nullptr);
    vkCmdPushConstants(cmd, pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &count_val);
    vkCmdDispatchIndirect(cmd, buf_ind, 0);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VK_CHECK(vkResetFences(device, 1, &fence));
    VK_CHECK(vkQueueSubmit(queue, 1, &submit, fence));
    VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

    VK_CHECK(vkMapMemory(device, mem_out, 0, buf_size, 0, (void**)&out_ptr));
    bool indirect_pass = true;
    for (int i = 0; i < N; i++) {
        float expected = (float)i * 1.5f + (float)(N - i) * 0.5f;
        if (std::abs(out_ptr[i] - expected) > 1e-4f) {
            std::cerr << "[FAIL] DispatchIndirect mismatch at " << i << ": got " << out_ptr[i] << ", expected " << expected << std::endl;
            indirect_pass = false;
            break;
        }
    }
    vkUnmapMemory(device, mem_out);

    if (indirect_pass) {
        std::cout << "[PASS] vkCmdDispatchIndirect verified successfully!" << std::endl;
    }

    // 14. Cleanup
    vkDestroyBuffer(device, buf_ind, nullptr);
    vkFreeMemory(device, mem_ind, nullptr);
    vkDestroyFence(device, fence, nullptr);
    vkDestroyDescriptorPool(device, desc_pool, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(device, set_layout, nullptr);
    vkDestroyShaderModule(device, shader_module, nullptr);
    vkDestroyBuffer(device, buf_a, nullptr);
    vkDestroyBuffer(device, buf_b, nullptr);
    vkDestroyBuffer(device, buf_out, nullptr);
    vkDestroyBuffer(device, buf_copy, nullptr);
    vkFreeMemory(device, mem_a, nullptr);
    vkFreeMemory(device, mem_b, nullptr);
    vkFreeMemory(device, mem_out, nullptr);
    vkFreeMemory(device, mem_copy, nullptr);
    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd);
    vkDestroyCommandPool(device, cmd_pool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    bool all_ok = compute_pass && fill_update_pass && indirect_pass;
    std::cout << (all_ok ? "[PASS] ALL RAW VULKAN API TESTS PASSED!" : "[FAIL] SOME VULKAN API TESTS FAILED!") << std::endl;
    return all_ok;
}

int main() {
    return test_vulkan_full_suite() ? 0 : 1;
}
