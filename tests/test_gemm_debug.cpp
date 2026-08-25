#include <iostream>
#include <vector>
#include <cmath>
#include <net.h>
#include <gpu.h>
#include <command.h>
#include <layer.h>
#include <layer_type.h>
#include <mat.h>

int main() {
    ncnn::create_gpu_instance();
    ncnn::VulkanDevice* vkdev = ncnn::get_gpu_device(0);

    for (int slm = 0; slm <= 1; slm++) {
        for (int sgemm = 0; sgemm <= 1; sgemm++) {
            ncnn::Option opt;
            opt.use_vulkan_compute = 1;
            opt.use_fp16_packed = 0;
            opt.use_fp16_storage = 0;
            opt.use_fp16_arithmetic = 0;
            opt.use_shader_local_memory = slm;
            opt.use_sgemm_convolution = sgemm;
            opt.blob_vkallocator = vkdev->acquire_blob_allocator();
            opt.staging_vkallocator = vkdev->acquire_staging_allocator();
            opt.workspace_vkallocator = opt.blob_vkallocator;

            int in_c = 16, out_c = 32, kernel = 3, stride = 2, pad = 1, w = 32, h = 32;

            ncnn::Layer* conv_gpu = ncnn::create_layer_vulkan(ncnn::LayerType::Convolution);
            ncnn::Layer* conv_cpu = ncnn::create_layer_cpu(ncnn::LayerType::Convolution);
            conv_gpu->vkdev = vkdev;

            ncnn::ParamDict pd;
            pd.set(0, out_c);
            pd.set(1, kernel);
            pd.set(2, 1);
            pd.set(3, stride);
            pd.set(4, pad);
            pd.set(5, 1);
            pd.set(6, in_c * out_c * kernel * kernel);

            conv_gpu->load_param(pd);
            conv_cpu->load_param(pd);

            ncnn::Mat weights[2];
            weights[0].create(in_c * out_c * kernel * kernel);
            weights[1].create(out_c);
            for (int i = 0; i < weights[0].total(); i++) weights[0][i] = ((i % 17) - 8) * 0.05f;
            for (int i = 0; i < weights[1].total(); i++) weights[1][i] = 0.1f * (i + 1);

            conv_gpu->load_model(ncnn::ModelBinFromMatArray(weights));
            conv_cpu->load_model(ncnn::ModelBinFromMatArray(weights));

            conv_gpu->create_pipeline(opt);
            ncnn::Option cpu_opt;
            conv_cpu->create_pipeline(cpu_opt);

            {
                ncnn::VkTransfer upload_cmd(vkdev);
                conv_gpu->upload_model(upload_cmd, opt);
                upload_cmd.submit_and_wait();
            }

            ncnn::Mat in(w, h, in_c);
            for (int q = 0; q < in_c; q++) {
                float* p = in.channel(q);
                for (int i = 0; i < w * h; i++) p[i] = ((i + q * 13) % 29) * 0.1f - 1.4f;
            }

            ncnn::Mat out_gpu_cpu;
            {
                ncnn::VkCompute cmd(vkdev);
                ncnn::VkMat in_gpu, out_gpu;
                cmd.record_upload(in, in_gpu, opt);
                conv_gpu->forward(in_gpu, out_gpu, cmd, opt);
                cmd.record_download(out_gpu, out_gpu_cpu, opt);
                cmd.submit_and_wait();
            }

            ncnn::Mat out_cpu;
            conv_cpu->forward(in, out_cpu, cpu_opt);

            ncnn::Mat out_gpu_unpacked;
            ncnn::convert_packing(out_gpu_cpu, out_gpu_unpacked, 1);

            float max_diff = 0.0f;
            for (int q = 0; q < out_cpu.c; q++) {
                const float* pa = out_gpu_unpacked.channel(q);
                const float* pb = out_cpu.channel(q);
                for (int i = 0; i < out_cpu.w * out_cpu.h; i++) {
                    float d = std::abs(pa[i] - pb[i]);
                    if (d > max_diff) max_diff = d;
                }
            }

            std::cout << "slm=" << slm << " sgemm=" << sgemm << " -> max_diff = " << max_diff << std::endl;

            conv_gpu->destroy_pipeline(opt);
            conv_cpu->destroy_pipeline(cpu_opt);
            delete conv_gpu;
            delete conv_cpu;

            vkdev->reclaim_blob_allocator(opt.blob_vkallocator);
            vkdev->reclaim_staging_allocator(opt.staging_vkallocator);
        }
    }

    ncnn::destroy_gpu_instance();
    return 0;
}
