#include <iostream>
#include <vector>
#include <net.h>
#include <mat.h>
#include <gpu.h>

class Warp : public ncnn::Layer {
public:
    Warp() {
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

        #pragma omp parallel for num_threads(opt.num_threads)
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

static ncnn::Layer* Warp_creator(void*) {
    return new Warp();
}

int main() {
    std::cout << "[TEST] Initializing ncnn Vulkan..." << std::endl;
    ncnn::create_gpu_instance();

    int gpu_count = ncnn::get_gpu_count();
    std::cout << "[TEST] GPU count: " << gpu_count << std::endl;
    if (gpu_count <= 0) {
        std::cerr << "[FAIL] No GPU found!" << std::endl;
        return 1;
    }

    ncnn::VulkanDevice* vkdev = ncnn::get_gpu_device(0);
    std::cout << "[TEST] Device name: " << vkdev->info.device_name() << std::endl;

    {
        ncnn::Net net;
        net.register_custom_layer("rife.Warp", Warp_creator);
        net.opt.use_vulkan_compute = true;
        net.opt.use_fp16_packed = false;
        net.opt.use_fp16_storage = false;
        net.opt.use_fp16_arithmetic = false;
        net.set_vulkan_device(vkdev);

        std::string param_path = "/home/user/repos/rife-ncnn-vulkan/models/rife-v4.6/flownet.param";
        std::string bin_path = "/home/user/repos/rife-ncnn-vulkan/models/rife-v4.6/flownet.bin";

        std::cout << "[TEST] Loading " << param_path << "..." << std::endl;
        if (net.load_param(param_path.c_str()) != 0) {
            std::cerr << "[FAIL] Failed to load param" << std::endl;
            return 1;
        }
        if (net.load_model(bin_path.c_str()) != 0) {
            std::cerr << "[FAIL] Failed to load bin" << std::endl;
            return 1;
        }
        std::cout << "[TEST] Loaded successfully! Running forward on dummy input..." << std::endl;

        // Create dummy input 64x64x3 float mat with varying content
        ncnn::Mat in0(64, 64, 3);
        for (int q = 0; q < 3; q++) {
            float* p = in0.channel(q);
            for (int y = 0; y < 64; y++) {
                for (int x = 0; x < 64; x++) {
                    p[y * 64 + x] = (float)x / 64.0f;
                }
            }
        }
        ncnn::Mat in1(64, 64, 3);
        for (int q = 0; q < 3; q++) {
            float* p = in1.channel(q);
            for (int y = 0; y < 64; y++) {
                for (int x = 0; x < 64; x++) {
                    p[y * 64 + x] = (float)y / 64.0f;
                }
            }
        }
        ncnn::Mat in2(64, 64, 1);
        in2.fill(0.5f);

        ncnn::Extractor ex = net.create_extractor();
        ex.input("in0", in0);
        ex.input("in1", in1);
        ex.input("in2", in2);

        ncnn::Mat out0;
        int ret = ex.extract("out0", out0);
        std::cout << "[TEST GPU] Extract out0 return code: " << ret << " (dims=" << out0.dims << ", w=" << out0.w << ", h=" << out0.h << ", c=" << out0.c << ")" << std::endl;

        if (out0.empty()) {
            std::cerr << "[FAIL] out0 is empty!" << std::endl;
            return 1;
        }

        float min_val = 1e9f, max_val = -1e9f, sum = 0.f;
        int total = out0.total();
        for (int i = 0; i < total; i++) {
            float v = out0[i];
            if (v < min_val) min_val = v;
            if (v > max_val) max_val = v;
            sum += v;
        }
        std::cout << "[GPU RESULT] total=" << total << " min=" << min_val << " max=" << max_val << " mean=" << (sum / total) << std::endl;

        for (int c = 0; c < out0.c; c++) {
            const ncnn::Mat ch = out0.channel(c);
            float ch_sum = 0.f;
            for (int i = 0; i < ch.total(); i++) ch_sum += ch[i];
            std::cout << "  -> GPU Channel " << c << " mean=" << (ch_sum / ch.total()) << std::endl;
        }

        // Now run CPU
        ncnn::Net cpu_net;
        cpu_net.register_custom_layer("rife.Warp", Warp_creator);
        cpu_net.load_param(param_path.c_str());
        cpu_net.load_model(bin_path.c_str());

        ncnn::Extractor cpu_ex = cpu_net.create_extractor();
        cpu_ex.input("in0", in0);
        cpu_ex.input("in1", in1);
        cpu_ex.input("in2", in2);

        ncnn::Mat cpu_out0;
        cpu_ex.extract("out0", cpu_out0);

        for (int c = 0; c < cpu_out0.c; c++) {
            const ncnn::Mat ch = cpu_out0.channel(c);
            float ch_sum = 0.f;
            for (int i = 0; i < ch.total(); i++) ch_sum += ch[i];
            std::cout << "  -> CPU Channel " << c << " mean=" << (ch_sum / ch.total()) << std::endl;
        }
    }

    ncnn::destroy_gpu_instance();
    std::cout << "[PASS] test_rife_model finished successfully!" << std::endl;
    return 0;
}
