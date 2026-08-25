#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <cstring>
#include <net.h>
#include <mat.h>
#include <gpu.h>

#define STB_IMAGE_IMPLEMENTATION
#include "/tmp/ncnn/src/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "/tmp/ncnn/src/stb_image_write.h"

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

static ncnn::Layer* Warp_creator(void*) { return new Warp(); }

int main(int argc, char** argv) {
    std::string input0_path = "/home/user/repos/glvk/rife-ncnn-vulkan-20221029-ubuntu/inputs/00000072.png";
    std::string input1_path = "/home/user/repos/glvk/rife-ncnn-vulkan-20221029-ubuntu/inputs/00000073.png";
    std::string model_dir = "/home/user/repos/glvk/rife-ncnn-vulkan-20221029-ubuntu/rife-v4.6";
    std::string output_path = "/tmp/glvk_rife_interpolated.png";
    float timestep = 0.5f;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-0") == 0 || strcmp(argv[i], "--input0") == 0) && i + 1 < argc) {
            input0_path = argv[++i];
        } else if ((strcmp(argv[i], "-1") == 0 || strcmp(argv[i], "--input1") == 0) && i + 1 < argc) {
            input1_path = argv[++i];
        } else if ((strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--model") == 0) && i + 1 < argc) {
            model_dir = argv[++i];
        } else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc) {
            output_path = argv[++i];
        } else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timestep") == 0) && i + 1 < argc) {
            timestep = std::stof(argv[++i]);
        }
    }

    int img_w, img_h, img_c;
    unsigned char* data0 = stbi_load(input0_path.c_str(), &img_w, &img_h, &img_c, 3);
    unsigned char* data1 = stbi_load(input1_path.c_str(), &img_w, &img_h, &img_c, 3);
    if (!data0 || !data1) {
        std::cerr << "Failed to load input images: " << input0_path << " / " << input1_path << std::endl;
        return 1;
    }
    std::cout << "Loaded input frames: " << img_w << "x" << img_h << " (3 channels)" << std::endl;

    // Pad to multiple of 32
    int pw = (img_w + 31) / 32 * 32;
    int ph = (img_h + 31) / 32 * 32;

    ncnn::Mat in0(pw, ph, 3);
    ncnn::Mat in1(pw, ph, 3);
    in0.fill(0.0f);
    in1.fill(0.0f);

    for (int q = 0; q < 3; q++) {
        float* p0 = in0.channel(q);
        float* p1 = in1.channel(q);
        for (int y = 0; y < img_h; y++) {
            for (int x = 0; x < img_w; x++) {
                int src_idx = (y * img_w + x) * 3 + q;
                int dst_idx = y * pw + x;
                p0[dst_idx] = data0[src_idx] * (1.0f / 255.0f);
                p1[dst_idx] = data1[src_idx] * (1.0f / 255.0f);
            }
        }
    }

    ncnn::Mat in2(1, 1, 1);
    in2.fill(timestep);

    std::string param_path = model_dir + "/flownet.param";
    std::string bin_path = model_dir + "/flownet.bin";

    ncnn::create_gpu_instance();
    ncnn::VulkanDevice* vkdev = ncnn::get_gpu_device(0);

    ncnn::Mat gpu_out0;
    {
        ncnn::Net net;
        net.register_custom_layer("rife.Warp", Warp_creator);
        net.opt.use_vulkan_compute = true;
        net.set_vulkan_device(vkdev);
        if (net.load_param(param_path.c_str()) != 0) {
            std::cerr << "Failed to load param: " << param_path << std::endl;
            return 1;
        }
        if (net.load_model(bin_path.c_str()) != 0) {
            std::cerr << "Failed to load model: " << bin_path << std::endl;
            return 1;
        }

        ncnn::Extractor ex = net.create_extractor();
        ex.input("in0", in0);
        ex.input("in1", in1);
        ex.input("in2", in2);
        ex.extract("out0", gpu_out0);
    }

    std::vector<unsigned char> out_pixels(img_w * img_h * 3);
    for (int y = 0; y < img_h; y++) {
        for (int x = 0; x < img_w; x++) {
            int src_idx = y * pw + x;
            int dst_idx = (y * img_w + x) * 3;
            for (int q = 0; q < 3; q++) {
                float v = gpu_out0.channel(q)[src_idx] * 255.0f + 0.5f;
                int vi = std::max(0, std::min(255, (int)std::floor(v)));
                out_pixels[dst_idx + q] = (unsigned char)vi;
            }
        }
    }

    stbi_write_png(output_path.c_str(), img_w, img_h, 3, out_pixels.data(), img_w * 3);
    std::cout << "Interpolated image saved to: " << output_path << " (" << img_w << "x" << img_h << ")" << std::endl;

    stbi_image_free(data0);
    stbi_image_free(data1);
    ncnn::destroy_gpu_instance();
    return 0;
}
