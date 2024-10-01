#include "pollux_decode.h"
#include "pollux_fmt.h"
#include "pollux_erron.h"

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <vector>

constexpr unsigned int YUV_GRP = 1024;

const static std::string video_1 = "./input1_1280-720_video_audio.mp4";
const static std::string video_2 = "./input2_2560-1440_video.mp4";
const static std::string video_3 = "./input3_3506-2200_video.avi";

static int ng = 1;

class pollux_test1 {
public:
    pollux_test1() : p_pollux(nullptr) {
        if (pollux_decode_init(&p_pollux)) {
            throw std::runtime_error("error, pollux_decode_init");
        }
    }

    ~pollux_test1() {
        if (p_pollux) {
            p_pollux->release(p_pollux);
            pollux_decode_deinit(p_pollux);
        }
    }

    inline unsigned int
    img_buf_size(pollux_fmt_t fmt, int height, int stride)
    {
        switch (fmt) {
            case POLLUX_FMT_444P:
                return height * stride * 3;
            case POLLUX_FMT_NV21:
            case POLLUX_FMT_NV12:
                return height * stride * 3 >> 1;
            default:
                std::cerr << "unsupported fmt: " << fmt << std::endl;
                return 0;
        }
    }

    int
    write_file(pollux_fmt_t format, pollux_decode_result_t* p_res)
    {
        int n = ng++;
        int ret;
        for (unsigned int i = 0; i < YUV_GRP; ++i) {
            ret = p_pollux->result_get(p_pollux, p_res);
            switch (ret) {
                case POLLUX_OK:
                    break;
                case POLLUX_ERR_FILE_END:
                    return 0;
                case POLLUX_ERR_DECODE_THD_EXIT:
                    return -1;
                default:
                    std::cerr << "warning, result_get: " << ret << std::endl;
                    continue;
            }

            std::string file_name =
                "1_" + std::to_string(n) + "_" +
                std::to_string(p_res->stride) + "-" +
                std::to_string(p_res->height) + "_" +
                std::to_string(i) + ".yuv";

            unsigned int buf_size =
                img_buf_size(format, p_res->height, p_res->stride);
            if (0 == buf_size) continue;
            FILE* file = fopen(file_name.c_str(), "wb");
            if (file) {
                fwrite(p_res->buf, 1, buf_size, file);
                fclose(file);
            } else {
                std::cerr << "warning, fopen failed" << std::endl;
                return -1;
            }
        }
        return 0;
    }

    int
    set_params_and_decode(
        pollux_fmt_t format, int height, int width,
        int alignment, int fps, const std::string& file,
        bool is_loop)
    {
        int ret;

        pollux_decode_yuv_t yuv = {};
        pollux_decode_param_t param = {};

        yuv.fmt = format;
        yuv.height = height;
        yuv.width = width;
        yuv.alignment = alignment;

        param.fps = fps;
        param.p_file = file.c_str();
        param.is_loop = is_loop;
        memcpy(&(param.yuv), &yuv, sizeof(pollux_decode_yuv_t));

        ret = p_pollux->param_set(p_pollux, &param);
        if (ret) {
            std::cerr << "error, param_set: " << ret << std::endl;
            return ret;
        }

        pollux_decode_result_t *p_res = nullptr;
        ret = pollux_decode_result_alloc(p_pollux, &p_res);
        if(ret) {
            std::cerr << "error, pollux_decode_result_alloc: " << ret << std::endl;
            return ret;
        }

        ret = write_file(format, p_res);
        pollux_decode_result_free(p_res);
        return ret;
    }

private:
    pollux_decode_t* p_pollux = nullptr;
};

int main()
{
    try {
        pollux_test1 decoder;

        if (decoder.set_params_and_decode(
            POLLUX_FMT_444P, 1080, 1920, 1, 60, video_1, true)) {
            return -1;
        }

        if (decoder.set_params_and_decode(
            POLLUX_FMT_NV21, 1440, 2560, 4, 30, video_2, false)) {
            return -1;
        }

        if (decoder.set_params_and_decode(
            POLLUX_FMT_NV12, 1100, 1753, 4, 30, video_3, true)) {
            return -1;
        }

    } catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
