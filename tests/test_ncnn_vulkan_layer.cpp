#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

// ncnn headers
#include <net.h>
#include <gpu.h>
#include <command.h>
#include <layer.h>
#include <layer_type.h>

bool test_relu_layer(ncnn::VulkanDevice* vkdev, bool use_fp16) {
    std::cout << "\n--- Testing Vulkan ReLU Layer (use_fp16=" << use_fp16 << ") ---" << std::endl;
    ncnn::Option opt;
    opt.use_vulkan_compute = 1;
    opt.use_fp16_packed = use_fp16;
    opt.use_fp16_storage = 0;
    opt.use_fp16_arithmetic = 0;
    opt.blob_vkallocator = vkdev->acquire_blob_allocator();
    opt.staging_vkallocator = vkdev->acquire_staging_allocator();

    ncnn::Layer* relu = ncnn::create_layer_vulkan(ncnn::LayerType::ReLU);
    assert(relu != nullptr);
    relu->vkdev = vkdev;

    ncnn::ParamDict pd;
    pd.set(0, 0.0f); // slope = 0
    relu->load_param(pd);
    int p_res = relu->create_pipeline(opt);
    std::cout << "[NCNN] relu->create_pipeline result: " << p_res << std::endl;

    const int N = 8;
    ncnn::Mat in(N);
    in[0] = -2.0f; in[1] = -1.0f; in[2] = 0.0f; in[3] = 1.0f;
    in[4] = 2.0f; in[5] = 3.0f; in[6] = -4.0f; in[7] = 5.0f;

    bool pass = true;
    {
        ncnn::VkCompute cmd(vkdev);
        ncnn::VkMat in_gpu;
        cmd.record_upload(in, in_gpu, opt);

        ncnn::VkMat out_gpu;
        relu->forward(in_gpu, out_gpu, cmd, opt);

        ncnn::Mat out;
        cmd.record_download(out_gpu, out, opt);
        cmd.submit_and_wait();

        std::cout << "[NCNN] ReLU Input:  ";
        for (int i = 0; i < N; i++) std::cout << in[i] << " ";
        std::cout << "\n[NCNN] ReLU Output: ";
        for (int i = 0; i < N; i++) std::cout << out[i] << " ";
        std::cout << std::endl;

        for (int i = 0; i < N; i++) {
            float expected = in[i] < 0.0f ? 0.0f : in[i];
            if (std::abs(out[i] - expected) > 1e-3f) {
                std::cerr << "Mismatch at " << i << ": got " << out[i] << ", expected " << expected << std::endl;
                pass = false;
            }
        }
    }

    relu->destroy_pipeline(opt);
    delete relu;

    vkdev->reclaim_blob_allocator(opt.blob_vkallocator);
    vkdev->reclaim_staging_allocator(opt.staging_vkallocator);

    return pass;
}

bool test_unaryop_layer(ncnn::VulkanDevice* vkdev, bool use_fp16) {
    std::cout << "\n--- Testing Vulkan Sigmoid Layer (UnaryOp, use_fp16=" << use_fp16 << ") ---" << std::endl;
    ncnn::Option opt;
    opt.use_vulkan_compute = 1;
    opt.use_fp16_packed = use_fp16;
    opt.use_fp16_storage = 0;
    opt.use_fp16_arithmetic = 0;
    opt.blob_vkallocator = vkdev->acquire_blob_allocator();
    opt.staging_vkallocator = vkdev->acquire_staging_allocator();

    ncnn::Layer* sigmoid = ncnn::create_layer_vulkan(ncnn::LayerType::Sigmoid);
    assert(sigmoid != nullptr);
    sigmoid->vkdev = vkdev;

    ncnn::ParamDict pd;
    sigmoid->load_param(pd);
    sigmoid->create_pipeline(opt);

    const int N = 4;
    ncnn::Mat in(N);
    in[0] = 0.0f;
    in[1] = 2.0f;
    in[2] = -2.0f;
    in[3] = 10.0f;

    bool pass = true;
    {
        ncnn::VkCompute cmd(vkdev);
        ncnn::VkMat in_gpu;
        cmd.record_upload(in, in_gpu, opt);

        ncnn::VkMat out_gpu;
        sigmoid->forward(in_gpu, out_gpu, cmd, opt);

        ncnn::Mat out;
        cmd.record_download(out_gpu, out, opt);
        cmd.submit_and_wait();

        std::cout << "[NCNN] Sigmoid Input:  ";
        for (int i = 0; i < N; i++) std::cout << in[i] << " ";
        std::cout << "\n[NCNN] Sigmoid Output: ";
        for (int i = 0; i < N; i++) std::cout << out[i] << " ";
        std::cout << std::endl;

        for (int i = 0; i < N; i++) {
            float expected = 1.0f / (1.0f + std::exp(-in[i]));
            if (std::abs(out[i] - expected) > 1e-3f) {
                std::cerr << "Mismatch at " << i << ": got " << out[i] << ", expected " << expected << std::endl;
                pass = false;
            }
        }
    }

    sigmoid->destroy_pipeline(opt);
    delete sigmoid;

    vkdev->reclaim_blob_allocator(opt.blob_vkallocator);
    vkdev->reclaim_staging_allocator(opt.staging_vkallocator);

    return pass;
}

int main() {
    std::cout << "[NCNN-TEST] Initializing ncnn GPU instance..." << std::endl;
    int res = ncnn::create_gpu_instance();
    std::cout << "[NCNN-TEST] ncnn::create_gpu_instance result: " << res << std::endl;

    int gpu_count = ncnn::get_gpu_count();
    std::cout << "[NCNN-TEST] GPU Count: " << gpu_count << std::endl;
    if (gpu_count <= 0) {
        std::cerr << "[NCNN-TEST] No GPU found via ncnn." << std::endl;
        ncnn::destroy_gpu_instance();
        return 1;
    }

    const ncnn::GpuInfo& info = ncnn::get_gpu_info(0);
    std::cout << "[NCNN-TEST] GPU Device Name: " << info.device_name() << std::endl;

    ncnn::VulkanDevice* vkdev = ncnn::get_gpu_device(0);
    assert(vkdev != nullptr);

    bool all_passed = true;
    all_passed &= test_relu_layer(vkdev, false);
    all_passed &= test_unaryop_layer(vkdev, false);
    all_passed &= test_relu_layer(vkdev, true);
    all_passed &= test_unaryop_layer(vkdev, true);

    ncnn::destroy_gpu_instance();

    if (all_passed) {
        std::cout << "\n[NCNN-TEST] ALL TESTS PASSED: GLVK successfully executed ncnn Vulkan compute kernels on OpenGL 4.3+!" << std::endl;
        return 0;
    } else {
        std::cerr << "\n[NCNN-TEST] SOME TESTS FAILED!" << std::endl;
        return 1;
    }
}
