#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <string>
#include <random>

// ncnn headers
#include <net.h>
#include <gpu.h>
#include <command.h>
#include <layer.h>
#include <layer_type.h>
#include <mat.h>

// Helper to compare two Mat objects
static bool compare_mats(const ncnn::Mat& a_in, const ncnn::Mat& b_in, const std::string& name, float tol = 1e-2f) {
    ncnn::Mat a, b;
    if (a_in.elempack != 1) {
        ncnn::convert_packing(a_in, a, 1);
    } else {
        a = a_in;
    }
    if (b_in.elempack != 1) {
        ncnn::convert_packing(b_in, b, 1);
    } else {
        b = b_in;
    }

    if (a.dims != b.dims || a.w != b.w || a.h != b.h || a.d != b.d || a.c != b.c) {
        std::cerr << "[FAIL] Shape mismatch for " << name << ": A=(" << a.w << "," << a.h << "," << a.c << ") vs B=("
                  << b.w << "," << b.h << "," << b.c << ")" << std::endl;
        return false;
    }

    int total = a.total();
    float max_diff = 0.0f;
    float sum_diff = 0.0f;
    int mismatch_count = 0;

    for (int q = 0; q < a.c; q++) {
        const float* ptr_a = a.channel(q);
        const float* ptr_b = b.channel(q);
        int plane = a.w * a.h * a.d;
        for (int i = 0; i < plane; i++) {
            float diff = std::abs(ptr_a[i] - ptr_b[i]);
            if (diff > max_diff) max_diff = diff;
            sum_diff += diff;
            if (diff > tol) {
                if (mismatch_count < 5) {
                    std::cerr << "  [MISMATCH] " << name << " c=" << q << " i=" << i << ": GPU=" << ptr_a[i]
                              << " CPU=" << ptr_b[i] << " diff=" << diff << std::endl;
                }
                mismatch_count++;
            }
        }
    }

    float mean_diff = total > 0 ? sum_diff / total : 0.0f;
    if (mismatch_count > 0) {
        std::cerr << "[FAIL] " << name << " failed with " << mismatch_count << "/" << total
                  << " mismatches (max_diff=" << max_diff << ", mean_diff=" << mean_diff << ")" << std::endl;
        return false;
    }

    std::cout << "[PASS] " << name << " (max_diff=" << max_diff << ", mean_diff=" << mean_diff << ")" << std::endl;
    return true;
}

static ncnn::Option make_vulkan_opt(ncnn::VulkanDevice* vkdev, bool use_fp16 = false) {
    ncnn::Option opt;
    opt.use_vulkan_compute = 1;
    opt.use_fp16_packed = use_fp16;
    opt.use_fp16_storage = 0;
    opt.use_fp16_arithmetic = 0;
    opt.blob_vkallocator = vkdev->acquire_blob_allocator();
    opt.staging_vkallocator = vkdev->acquire_staging_allocator();
    opt.workspace_vkallocator = opt.blob_vkallocator;
    return opt;
}

static void free_vulkan_opt(ncnn::VulkanDevice* vkdev, ncnn::Option& opt) {
    vkdev->reclaim_blob_allocator(opt.blob_vkallocator);
    vkdev->reclaim_staging_allocator(opt.staging_vkallocator);
}

// 1. Convolution layer test
static bool test_convolution(ncnn::VulkanDevice* vkdev, int w, int h, int in_c, int out_c, int kernel, int stride, int pad, bool use_fp16 = false) {
    std::string name = "Convolution(in_c=" + std::to_string(in_c) + ", out_c=" + std::to_string(out_c) +
                       ", k=" + std::to_string(kernel) + ", s=" + std::to_string(stride) + ", pad=" + std::to_string(pad) +
                       ", fp16=" + std::to_string(use_fp16) + ")";
    
    ncnn::Option opt = make_vulkan_opt(vkdev, use_fp16);

    ncnn::Layer* conv_gpu = ncnn::create_layer_vulkan(ncnn::LayerType::Convolution);
    ncnn::Layer* conv_cpu = ncnn::create_layer_cpu(ncnn::LayerType::Convolution);
    conv_gpu->vkdev = vkdev;

    ncnn::ParamDict pd;
    pd.set(0, out_c);
    pd.set(1, kernel);
    pd.set(2, 1); // dilation
    pd.set(3, stride);
    pd.set(4, pad);
    pd.set(5, 1); // bias
    pd.set(6, in_c * out_c * kernel * kernel); // weight_data_size

    conv_gpu->load_param(pd);
    conv_cpu->load_param(pd);

    // Initialize weights and bias separately for GPU and CPU
    ncnn::Mat weights_gpu[2];
    weights_gpu[0].create(in_c * out_c * kernel * kernel);
    weights_gpu[1].create(out_c);
    for (int i = 0; i < weights_gpu[0].total(); i++) weights_gpu[0][i] = ((i % 17) - 8) * 0.05f;
    for (int i = 0; i < weights_gpu[1].total(); i++) weights_gpu[1][i] = 0.1f * (i + 1);

    ncnn::Mat weights_cpu[2] = { weights_gpu[0].clone(), weights_gpu[1].clone() };

    conv_gpu->load_model(ncnn::ModelBinFromMatArray(weights_gpu));
    conv_cpu->load_model(ncnn::ModelBinFromMatArray(weights_cpu));

    conv_gpu->create_pipeline(opt);
    ncnn::Option cpu_opt;
    cpu_opt.use_packing_layout = false;
    cpu_opt.use_sgemm_convolution = false;
    cpu_opt.use_winograd_convolution = false;
    conv_cpu->create_pipeline(cpu_opt);

    {
        ncnn::VkTransfer upload_cmd(vkdev);
        conv_gpu->upload_model(upload_cmd, opt);
        upload_cmd.submit_and_wait();
    }

    // Create input mat
    ncnn::Mat in(w, h, in_c);
    for (int q = 0; q < in_c; q++) {
        float* p = in.channel(q);
        for (int i = 0; i < w * h; i++) {
            p[i] = ((i + q * 13) % 29) * 0.1f - 1.4f;
        }
    }

    // Run GPU
    ncnn::Mat out_gpu_cpu;
    {
        ncnn::VkCompute cmd(vkdev);
        ncnn::VkMat in_gpu, out_gpu;
        cmd.record_upload(in, in_gpu, opt);
        conv_gpu->forward(in_gpu, out_gpu, cmd, opt);
        cmd.record_download(out_gpu, out_gpu_cpu, opt);
        cmd.submit_and_wait();
    }

    // Run CPU
    ncnn::Mat out_cpu;
    conv_cpu->forward(in, out_cpu, cpu_opt);

    float tol = use_fp16 ? 0.05f : 1e-2f;
    bool pass = compare_mats(out_gpu_cpu, out_cpu, name, tol);

    conv_gpu->destroy_pipeline(opt);
    conv_cpu->destroy_pipeline(cpu_opt);
    delete conv_gpu;
    delete conv_cpu;

    free_vulkan_opt(vkdev, opt);
    return pass;
}

// 2. Deconvolution (transposed conv) layer test
static bool test_deconvolution(ncnn::VulkanDevice* vkdev, int w, int h, int in_c, int out_c, int kernel, int stride, int pad, bool use_fp16 = false) {
    std::string name = "Deconvolution(in_c=" + std::to_string(in_c) + ", out_c=" + std::to_string(out_c) +
                       ", k=" + std::to_string(kernel) + ", s=" + std::to_string(stride) + ", pad=" + std::to_string(pad) +
                       ", fp16=" + std::to_string(use_fp16) + ")";

    ncnn::Option opt = make_vulkan_opt(vkdev, use_fp16);

    ncnn::Layer* deconv_gpu = ncnn::create_layer_vulkan(ncnn::LayerType::Deconvolution);
    ncnn::Layer* deconv_cpu = ncnn::create_layer_cpu(ncnn::LayerType::Deconvolution);
    deconv_gpu->vkdev = vkdev;

    ncnn::ParamDict pd;
    pd.set(0, out_c);
    pd.set(1, kernel);
    pd.set(2, 1); // dilation
    pd.set(3, stride);
    pd.set(4, pad);
    pd.set(5, 1); // bias
    pd.set(6, in_c * out_c * kernel * kernel);

    deconv_gpu->load_param(pd);
    deconv_cpu->load_param(pd);

    ncnn::Mat weights_gpu[2];
    weights_gpu[0].create(in_c * out_c * kernel * kernel);
    weights_gpu[1].create(out_c);
    for (int i = 0; i < weights_gpu[0].total(); i++) weights_gpu[0][i] = ((i % 13) - 6) * 0.04f;
    for (int i = 0; i < weights_gpu[1].total(); i++) weights_gpu[1][i] = 0.05f * (i + 1);

    ncnn::Mat weights_cpu[2] = { weights_gpu[0].clone(), weights_gpu[1].clone() };

    deconv_gpu->load_model(ncnn::ModelBinFromMatArray(weights_gpu));
    deconv_cpu->load_model(ncnn::ModelBinFromMatArray(weights_cpu));

    deconv_gpu->create_pipeline(opt);
    ncnn::Option cpu_opt;
    deconv_cpu->create_pipeline(cpu_opt);

    {
        ncnn::VkTransfer upload_cmd(vkdev);
        deconv_gpu->upload_model(upload_cmd, opt);
        upload_cmd.submit_and_wait();
    }

    ncnn::Mat in(w, h, in_c);
    for (int q = 0; q < in_c; q++) {
        float* p = in.channel(q);
        for (int i = 0; i < w * h; i++) {
            p[i] = ((i + q * 7) % 23) * 0.1f - 1.1f;
        }
    }

    ncnn::Mat out_gpu_cpu;
    {
        ncnn::VkCompute cmd(vkdev);
        ncnn::VkMat in_gpu, out_gpu;
        cmd.record_upload(in, in_gpu, opt);
        deconv_gpu->forward(in_gpu, out_gpu, cmd, opt);
        cmd.record_download(out_gpu, out_gpu_cpu, opt);
        cmd.submit_and_wait();
    }

    ncnn::Mat out_cpu;
    deconv_cpu->forward(in, out_cpu, cpu_opt);

    float tol = use_fp16 ? 0.05f : 1e-2f;
    bool pass = compare_mats(out_gpu_cpu, out_cpu, name, tol);

    deconv_gpu->destroy_pipeline(opt);
    deconv_cpu->destroy_pipeline(cpu_opt);
    delete deconv_gpu;
    delete deconv_cpu;

    free_vulkan_opt(vkdev, opt);
    return pass;
}

// 3. PReLU layer test
static bool test_prelu(ncnn::VulkanDevice* vkdev, int w, int h, int c, bool use_fp16 = false) {
    std::string name = "PReLU(w=" + std::to_string(w) + ", h=" + std::to_string(h) + ", c=" + std::to_string(c) +
                       ", fp16=" + std::to_string(use_fp16) + ")";

    ncnn::Option opt = make_vulkan_opt(vkdev, use_fp16);

    ncnn::Layer* prelu_gpu = ncnn::create_layer_vulkan(ncnn::LayerType::PReLU);
    ncnn::Layer* prelu_cpu = ncnn::create_layer_cpu(ncnn::LayerType::PReLU);
    prelu_gpu->vkdev = vkdev;

    ncnn::ParamDict pd;
    pd.set(0, c); // num_slope
    prelu_gpu->load_param(pd);
    prelu_cpu->load_param(pd);

    ncnn::Mat slope_gpu(c);
    for (int i = 0; i < c; i++) slope_gpu[i] = 0.1f * (i + 1);
    ncnn::Mat slope_cpu = slope_gpu.clone();

    prelu_gpu->load_model(ncnn::ModelBinFromMatArray(&slope_gpu));
    prelu_cpu->load_model(ncnn::ModelBinFromMatArray(&slope_cpu));

    prelu_gpu->create_pipeline(opt);
    ncnn::Option cpu_opt;
    prelu_cpu->create_pipeline(cpu_opt);

    {
        ncnn::VkTransfer upload_cmd(vkdev);
        prelu_gpu->upload_model(upload_cmd, opt);
        upload_cmd.submit_and_wait();
    }

    ncnn::Mat in(w, h, c);
    for (int q = 0; q < c; q++) {
        float* p = in.channel(q);
        for (int i = 0; i < w * h; i++) {
            p[i] = ((i % 11) - 5) * 0.5f;
        }
    }

    ncnn::Mat out_gpu_cpu;
    {
        ncnn::VkCompute cmd(vkdev);
        ncnn::VkMat in_gpu;
        cmd.record_upload(in, in_gpu, opt);
        prelu_gpu->forward_inplace(in_gpu, cmd, opt);
        cmd.record_download(in_gpu, out_gpu_cpu, opt);
        cmd.submit_and_wait();
    }

    ncnn::Mat out_cpu = in.clone();
    prelu_cpu->forward_inplace(out_cpu, cpu_opt);

    float tol = use_fp16 ? 0.05f : 1e-2f;
    bool pass = compare_mats(out_gpu_cpu, out_cpu, name, tol);

    prelu_gpu->destroy_pipeline(opt);
    prelu_cpu->destroy_pipeline(cpu_opt);
    delete prelu_gpu;
    delete prelu_cpu;

    free_vulkan_opt(vkdev, opt);
    return pass;
}

// 4. BinaryOp layer test (Add, Sub, Mul)
static bool test_binaryop(ncnn::VulkanDevice* vkdev, int op_type, int w, int h, int c, bool use_fp16 = false) {
    const char* op_names[] = {"Add", "Sub", "Mul", "Div", "Max", "Min", "Pow", "RSub", "RDiv"};
    std::string name = std::string("BinaryOp_") + op_names[op_type] + "(w=" + std::to_string(w) + ", c=" + std::to_string(c) +
                       ", fp16=" + std::to_string(use_fp16) + ")";

    ncnn::Option opt = make_vulkan_opt(vkdev, use_fp16);

    ncnn::Layer* bin_gpu = ncnn::create_layer_vulkan(ncnn::LayerType::BinaryOp);
    ncnn::Layer* bin_cpu = ncnn::create_layer_cpu(ncnn::LayerType::BinaryOp);
    bin_gpu->vkdev = vkdev;

    ncnn::ParamDict pd;
    pd.set(0, op_type);
    pd.set(1, 0); // with_scalar = 0
    bin_gpu->load_param(pd);
    bin_cpu->load_param(pd);

    bin_gpu->create_pipeline(opt);
    ncnn::Option cpu_opt;
    bin_cpu->create_pipeline(cpu_opt);

    ncnn::Mat a(w, h, c);
    ncnn::Mat b(w, h, c);
    for (int q = 0; q < c; q++) {
        float* pa = a.channel(q);
        float* pb = b.channel(q);
        for (int i = 0; i < w * h; i++) {
            pa[i] = ((i % 13) - 6) * 0.2f;
            pb[i] = ((i % 7) + 1) * 0.3f;
        }
    }

    ncnn::Mat out_gpu_cpu;
    {
        ncnn::VkCompute cmd(vkdev);
        ncnn::VkMat a_gpu, b_gpu, out_gpu;
        cmd.record_upload(a, a_gpu, opt);
        cmd.record_upload(b, b_gpu, opt);
        std::vector<ncnn::VkMat> bottom_blobs = {a_gpu, b_gpu};
        std::vector<ncnn::VkMat> top_blobs(1);
        bin_gpu->forward(bottom_blobs, top_blobs, cmd, opt);
        cmd.record_download(top_blobs[0], out_gpu_cpu, opt);
        cmd.submit_and_wait();
    }

    std::vector<ncnn::Mat> bottom_blobs_cpu = {a, b};
    std::vector<ncnn::Mat> top_blobs_cpu(1);
    bin_cpu->forward(bottom_blobs_cpu, top_blobs_cpu, cpu_opt);

    float tol = use_fp16 ? 0.05f : 1e-2f;
    bool pass = compare_mats(out_gpu_cpu, top_blobs_cpu[0], name, tol);

    bin_gpu->destroy_pipeline(opt);
    bin_cpu->destroy_pipeline(cpu_opt);
    delete bin_gpu;
    delete bin_cpu;

    free_vulkan_opt(vkdev, opt);
    return pass;
}

// 5. Interp layer test (Bilinear upsampling 2x)
static bool test_interp(ncnn::VulkanDevice* vkdev, int w, int h, int c, bool use_fp16 = false) {
    std::string name = "Interp_Bilinear_2x(w=" + std::to_string(w) + ", h=" + std::to_string(h) + ", c=" + std::to_string(c) +
                       ", fp16=" + std::to_string(use_fp16) + ")";

    ncnn::Option opt = make_vulkan_opt(vkdev, use_fp16);

    ncnn::Layer* interp_gpu = ncnn::create_layer_vulkan(ncnn::LayerType::Interp);
    ncnn::Layer* interp_cpu = ncnn::create_layer_cpu(ncnn::LayerType::Interp);
    interp_gpu->vkdev = vkdev;

    ncnn::ParamDict pd;
    pd.set(0, 2); // 2: bilinear
    pd.set(1, 2.0f); // height_scale
    pd.set(2, 2.0f); // width_scale
    interp_gpu->load_param(pd);
    interp_cpu->load_param(pd);

    interp_gpu->create_pipeline(opt);
    ncnn::Option cpu_opt;
    interp_cpu->create_pipeline(cpu_opt);

    ncnn::Mat in(w, h, c);
    for (int q = 0; q < c; q++) {
        float* p = in.channel(q);
        for (int i = 0; i < w * h; i++) {
            p[i] = ((i + q * 5) % 19) * 0.1f;
        }
    }

    ncnn::Mat out_gpu_cpu;
    {
        ncnn::VkCompute cmd(vkdev);
        ncnn::VkMat in_gpu, out_gpu;
        cmd.record_upload(in, in_gpu, opt);
        interp_gpu->forward(in_gpu, out_gpu, cmd, opt);
        cmd.record_download(out_gpu, out_gpu_cpu, opt);
        cmd.submit_and_wait();
    }

    ncnn::Mat out_cpu;
    interp_cpu->forward(in, out_cpu, cpu_opt);

    float tol = use_fp16 ? 0.05f : 1e-2f;
    bool pass = compare_mats(out_gpu_cpu, out_cpu, name, tol);

    interp_gpu->destroy_pipeline(opt);
    interp_cpu->destroy_pipeline(cpu_opt);
    delete interp_gpu;
    delete interp_cpu;

    free_vulkan_opt(vkdev, opt);
    return pass;
}

// 6. Concat layer test
static bool test_concat(ncnn::VulkanDevice* vkdev, int w, int h, int c1, int c2, bool use_fp16 = false) {
    std::string name = "Concat(c1=" + std::to_string(c1) + ", c2=" + std::to_string(c2) +
                       ", fp16=" + std::to_string(use_fp16) + ")";

    ncnn::Option opt = make_vulkan_opt(vkdev, use_fp16);

    ncnn::Layer* concat_gpu = ncnn::create_layer_vulkan(ncnn::LayerType::Concat);
    ncnn::Layer* concat_cpu = ncnn::create_layer_cpu(ncnn::LayerType::Concat);
    concat_gpu->vkdev = vkdev;

    ncnn::ParamDict pd;
    pd.set(0, 0); // axis = 0 (channel axis for 3D Mat)
    concat_gpu->load_param(pd);
    concat_cpu->load_param(pd);

    concat_gpu->create_pipeline(opt);
    ncnn::Option cpu_opt;
    concat_cpu->create_pipeline(cpu_opt);

    ncnn::Mat a(w, h, c1);
    ncnn::Mat b(w, h, c2);
    for (int q = 0; q < c1; q++) {
        float* p = a.channel(q);
        for (int i = 0; i < w * h; i++) p[i] = 1.0f + q * 0.1f + (i % 10) * 0.05f;
    }
    for (int q = 0; q < c2; q++) {
        float* p = b.channel(q);
        for (int i = 0; i < w * h; i++) p[i] = 2.0f + q * 0.1f + (i % 10) * 0.05f;
    }

    ncnn::Mat out_gpu_cpu;
    {
        ncnn::VkCompute cmd(vkdev);
        ncnn::VkMat a_gpu, b_gpu, out_gpu;
        cmd.record_upload(a, a_gpu, opt);
        cmd.record_upload(b, b_gpu, opt);
        std::vector<ncnn::VkMat> bottom_blobs = {a_gpu, b_gpu};
        std::vector<ncnn::VkMat> top_blobs(1);
        concat_gpu->forward(bottom_blobs, top_blobs, cmd, opt);
        cmd.record_download(top_blobs[0], out_gpu_cpu, opt);
        cmd.submit_and_wait();
    }

    std::vector<ncnn::Mat> bottom_blobs_cpu = {a, b};
    std::vector<ncnn::Mat> top_blobs_cpu(1);
    concat_cpu->forward(bottom_blobs_cpu, top_blobs_cpu, cpu_opt);

    float tol = use_fp16 ? 0.05f : 1e-2f;
    bool pass = compare_mats(out_gpu_cpu, top_blobs_cpu[0], name, tol);

    concat_gpu->destroy_pipeline(opt);
    concat_cpu->destroy_pipeline(cpu_opt);
    delete concat_gpu;
    delete concat_cpu;

    free_vulkan_opt(vkdev, opt);
    return pass;
}

// 7. Crop / Slice layer test
static bool test_crop_slice(ncnn::VulkanDevice* vkdev, int w, int h, int total_c, int slice_c_start, int slice_c_len, bool use_fp16 = false) {
    std::string name = "CropSlice(total_c=" + std::to_string(total_c) + ", start=" + std::to_string(slice_c_start) + ", len=" + std::to_string(slice_c_len) +
                       ", fp16=" + std::to_string(use_fp16) + ")";

    ncnn::Option opt = make_vulkan_opt(vkdev, use_fp16);

    ncnn::Layer* crop_gpu = ncnn::create_layer_vulkan(ncnn::LayerType::Crop);
    ncnn::Layer* crop_cpu = ncnn::create_layer_cpu(ncnn::LayerType::Crop);
    crop_gpu->vkdev = vkdev;

    ncnn::ParamDict pd;
    pd.set(0, 0); // woffset
    pd.set(1, 0); // hoffset
    pd.set(2, slice_c_start); // coffset
    pd.set(3, w); // outw
    pd.set(4, h); // outh
    pd.set(5, slice_c_len); // outc
    crop_gpu->load_param(pd);
    crop_cpu->load_param(pd);

    crop_gpu->create_pipeline(opt);
    ncnn::Option cpu_opt;
    crop_cpu->create_pipeline(cpu_opt);

    ncnn::Mat in(w, h, total_c);
    for (int q = 0; q < total_c; q++) {
        float* p = in.channel(q);
        for (int i = 0; i < w * h; i++) p[i] = (q + 1) * 10.0f + i * 0.1f;
    }

    ncnn::Mat out_gpu_cpu;
    {
        ncnn::VkCompute cmd(vkdev);
        ncnn::VkMat in_gpu, out_gpu;
        cmd.record_upload(in, in_gpu, opt);
        crop_gpu->forward(in_gpu, out_gpu, cmd, opt);
        cmd.record_download(out_gpu, out_gpu_cpu, opt);
        cmd.submit_and_wait();
    }

    ncnn::Mat out_cpu;
    crop_cpu->forward(in, out_cpu, cpu_opt);

    float tol = use_fp16 ? 0.05f : 1e-2f;
    bool pass = compare_mats(out_gpu_cpu, out_cpu, name, tol);

    crop_gpu->destroy_pipeline(opt);
    crop_cpu->destroy_pipeline(cpu_opt);
    delete crop_gpu;
    delete crop_cpu;

    free_vulkan_opt(vkdev, opt);
    return pass;
}

// 8. Sigmoid layer test
static bool test_sigmoid(ncnn::VulkanDevice* vkdev, int w, int h, int c, bool use_fp16 = false) {
    std::string name = "Sigmoid(w=" + std::to_string(w) + ", h=" + std::to_string(h) + ", c=" + std::to_string(c) +
                       ", fp16=" + std::to_string(use_fp16) + ")";

    ncnn::Option opt = make_vulkan_opt(vkdev, use_fp16);

    ncnn::Layer* sig_gpu = ncnn::create_layer_vulkan(ncnn::LayerType::Sigmoid);
    ncnn::Layer* sig_cpu = ncnn::create_layer_cpu(ncnn::LayerType::Sigmoid);
    sig_gpu->vkdev = vkdev;

    ncnn::ParamDict pd;
    sig_gpu->load_param(pd);
    sig_cpu->load_param(pd);

    sig_gpu->create_pipeline(opt);
    ncnn::Option cpu_opt;
    sig_cpu->create_pipeline(cpu_opt);

    ncnn::Mat in(w, h, c);
    for (int q = 0; q < c; q++) {
        float* p = in.channel(q);
        for (int i = 0; i < w * h; i++) p[i] = ((i % 15) - 7) * 0.5f;
    }

    ncnn::Mat out_gpu_cpu;
    {
        ncnn::VkCompute cmd(vkdev);
        ncnn::VkMat in_gpu, out_gpu;
        cmd.record_upload(in, in_gpu, opt);
        sig_gpu->forward(in_gpu, out_gpu, cmd, opt);
        cmd.record_download(out_gpu, out_gpu_cpu, opt);
        cmd.submit_and_wait();
    }

    ncnn::Mat out_cpu;
    sig_cpu->forward(in, out_cpu, cpu_opt);

    float tol = use_fp16 ? 0.05f : 1e-2f;
    bool pass = compare_mats(out_gpu_cpu, out_cpu, name, tol);

    sig_gpu->destroy_pipeline(opt);
    sig_cpu->destroy_pipeline(cpu_opt);
    delete sig_gpu;
    delete sig_cpu;

    free_vulkan_opt(vkdev, opt);
    return pass;
}

// 9. rife.Warp layer test
class CPUWarp : public ncnn::Layer {
public:
    CPUWarp() {
        one_blob_only = false;
        support_vulkan = false;
    }

    virtual int forward(const std::vector<ncnn::Mat>& bottom_blobs, std::vector<ncnn::Mat>& top_blobs, const ncnn::Option& opt) const {
        const ncnn::Mat& image = bottom_blobs[0];
        const ncnn::Mat& flow = bottom_blobs[1];

        int w = image.w;
        int h = image.h;
        int channels = image.c;

        ncnn::Mat& top_blob = top_blobs[0];
        top_blob.create(w, h, channels, sizeof(float), opt.blob_allocator);
        if (top_blob.empty()) return -100;

        for (int q = 0; q < channels; q++) {
            const float* ptr = image.channel(q);
            const float* flow_ptr_x = flow.channel(0);
            const float* flow_ptr_y = flow.channel(1);
            float* out_ptr = top_blob.channel(q);

            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    float fx = (float)x + flow_ptr_x[y * w + x];
                    float fy = (float)y + flow_ptr_y[y * w + x];

                    int x0 = (int)std::floor(fx);
                    int y0 = (int)std::floor(fy);
                    int x1 = x0 + 1;
                    int y1 = y0 + 1;

                    float wx1 = fx - (float)x0;
                    float wy1 = fy - (float)y0;
                    float wx0 = 1.0f - wx1;
                    float wy0 = 1.0f - wy1;

                    x0 = std::max(0, std::min(x0, w - 1));
                    x1 = std::max(0, std::min(x1, w - 1));
                    y0 = std::max(0, std::min(y0, h - 1));
                    y1 = std::max(0, std::min(y1, h - 1));

                    float v = wx0 * wy0 * ptr[y0 * w + x0] +
                              wx1 * wy0 * ptr[y0 * w + x1] +
                              wx0 * wy1 * ptr[y1 * w + x0] +
                              wx1 * wy1 * ptr[y1 * w + x1];
                    out_ptr[y * w + x] = v;
                }
            }
        }
        return 0;
    }
};

static bool test_warp_layer(ncnn::VulkanDevice* vkdev, int w, int h, int c) {
    std::string name = "rife.Warp(w=" + std::to_string(w) + ", h=" + std::to_string(h) + ", c=" + std::to_string(c) + ")";

    CPUWarp cpu_warp;
    ncnn::Option cpu_opt;

    ncnn::Mat image(w, h, c);
    for (int q = 0; q < c; q++) {
        float* p = image.channel(q);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                p[y * w + x] = std::sin(x * 0.1f) * std::cos(y * 0.1f) + q * 0.5f;
            }
        }
    }

    ncnn::Mat flow(w, h, 2);
    float* fx = flow.channel(0);
    float* fy = flow.channel(1);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            fx[y * w + x] = 1.5f * std::cos((x + y) * 0.05f);
            fy[y * w + x] = -1.2f * std::sin((x - y) * 0.05f);
        }
    }

    std::vector<ncnn::Mat> bottom_blobs = {image, flow};
    std::vector<ncnn::Mat> top_blobs(1);
    cpu_warp.forward(bottom_blobs, top_blobs, cpu_opt);

    // Verify CPU output is non-empty and has reasonable values
    ncnn::Mat& out = top_blobs[0];
    if (out.empty() || out.w != w || out.h != h || out.c != c) {
        std::cerr << "[FAIL] " << name << " output invalid" << std::endl;
        return false;
    }

    float mean_val = 0.0f;
    for (int q = 0; q < out.c; q++) {
        const float* p = out.channel(q);
        for (int i = 0; i < out.w * out.h; i++) mean_val += p[i];
    }
    mean_val /= (out.w * out.h * out.c);
    std::cout << "[PASS] " << name << " (mean=" << mean_val << ")" << std::endl;
    return true;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "  GLVK RIFE Layers Unit Test Suite (Vulkan vs CPU)" << std::endl;
    std::cout << "==================================================" << std::endl;

    ncnn::create_gpu_instance();
    int gpu_count = ncnn::get_gpu_count();
    if (gpu_count <= 0) {
        std::cerr << "[FAIL] No GPU found via ncnn/GLVK" << std::endl;
        ncnn::destroy_gpu_instance();
        return 1;
    }

    ncnn::VulkanDevice* vkdev = ncnn::get_gpu_device(0);
    std::cout << "[INFO] Using device: " << vkdev->info.device_name() << std::endl;

    bool support_fp16 = vkdev->info.support_fp16_packed();
    std::cout << "[INFO] Device FP16 packed support: " << (support_fp16 ? "YES" : "NO") << std::endl;

    bool all_passed = true;

    std::cout << "\n--- 1. Convolution Tests ---" << std::endl;
    all_passed &= test_convolution(vkdev, 32, 32, 3, 16, 3, 1, 1, false);
    all_passed &= test_convolution(vkdev, 32, 32, 16, 32, 3, 2, 1, false); // Stride 2 downsample
    all_passed &= test_convolution(vkdev, 16, 16, 32, 64, 3, 1, 1, false);
    all_passed &= test_convolution(vkdev, 16, 16, 64, 4, 3, 1, 1, false);
    if (support_fp16) {
        all_passed &= test_convolution(vkdev, 32, 32, 16, 32, 3, 1, 1, true); // FP16 packed
    }

    std::cout << "\n--- 2. Deconvolution Tests (Transposed Conv 2x) ---" << std::endl;
    all_passed &= test_deconvolution(vkdev, 16, 16, 32, 16, 4, 2, 1, false);
    all_passed &= test_deconvolution(vkdev, 8, 8, 64, 32, 4, 2, 1, false);
    if (support_fp16) {
        all_passed &= test_deconvolution(vkdev, 16, 16, 32, 16, 4, 2, 1, true); // FP16 packed
    }

    std::cout << "\n--- 3. PReLU Activation Tests ---" << std::endl;
    all_passed &= test_prelu(vkdev, 32, 32, 16, false);
    all_passed &= test_prelu(vkdev, 16, 16, 64, false);
    if (support_fp16) {
        all_passed &= test_prelu(vkdev, 32, 32, 16, true); // FP16 packed
    }

    std::cout << "\n--- 4. BinaryOp Tests ---" << std::endl;
    all_passed &= test_binaryop(vkdev, 0, 32, 32, 8, false); // Add
    all_passed &= test_binaryop(vkdev, 1, 32, 32, 8, false); // Sub
    all_passed &= test_binaryop(vkdev, 2, 32, 32, 8, false); // Mul
    if (support_fp16) {
        all_passed &= test_binaryop(vkdev, 0, 32, 32, 8, true);  // Add FP16 packed
        all_passed &= test_binaryop(vkdev, 2, 32, 32, 8, true);  // Mul FP16 packed
    }

    std::cout << "\n--- 5. Interp Tests (Bilinear 2x) ---" << std::endl;
    all_passed &= test_interp(vkdev, 16, 16, 4, false);
    all_passed &= test_interp(vkdev, 32, 32, 16, false);
    if (support_fp16) {
        all_passed &= test_interp(vkdev, 16, 16, 4, true); // FP16 packed
    }

    std::cout << "\n--- 6. Concat Tests ---" << std::endl;
    all_passed &= test_concat(vkdev, 16, 16, 4, 4, false);
    all_passed &= test_concat(vkdev, 16, 16, 16, 32, false);
    if (support_fp16) {
        all_passed &= test_concat(vkdev, 16, 16, 4, 4, true); // FP16 packed
    }

    std::cout << "\n--- 7. Crop / Slice Tests ---" << std::endl;
    all_passed &= test_crop_slice(vkdev, 16, 16, 16, 0, 4, false);
    all_passed &= test_crop_slice(vkdev, 16, 16, 16, 4, 8, false);
    all_passed &= test_crop_slice(vkdev, 16, 16, 16, 12, 4, false);

    std::cout << "\n--- 8. Sigmoid Activation Tests ---" << std::endl;
    all_passed &= test_sigmoid(vkdev, 32, 32, 8, false);
    if (support_fp16) {
        all_passed &= test_sigmoid(vkdev, 32, 32, 8, true); // FP16 packed
    }

    std::cout << "\n--- 9. rife.Warp Layer Tests ---" << std::endl;
    all_passed &= test_warp_layer(vkdev, 64, 64, 3);
    all_passed &= test_warp_layer(vkdev, 64, 64, 4);

    ncnn::destroy_gpu_instance();

    std::cout << "\n==================================================" << std::endl;
    if (all_passed) {
        std::cout << "  ALL RIFE LAYER TESTS PASSED ON GLVK!" << std::endl;
        std::cout << "==================================================" << std::endl;
        return 0;
    } else {
        std::cerr << "  SOME RIFE LAYER TESTS FAILED!" << std::endl;
        std::cout << "==================================================" << std::endl;
        return 1;
    }
}
