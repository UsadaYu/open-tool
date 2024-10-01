#include "pollux_decode.h"
#include "pollux_fmt.h"
#include "pollux_erron.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define YUV_NR (1024)
#define IS_LOOP (1)

const static char *video_1 = "./input2_2560-1440_video.mp4";

static int ng = 1;

static inline int
i_file_write(pollux_decode_t *p_pollux,
    pollux_decode_result_t *p_res)
{
    char file_name[64] = {0};
    FILE *file;
    int ret;
    int n = ng++;
    for (unsigned int i = 0; i < YUV_NR; i++) {
        ret = p_pollux->result_get(p_pollux, p_res);
        switch (ret) {
            case POLLUX_OK:
                break;
            case POLLUX_ERR_FILE_END:
                return 0;
            case POLLUX_ERR_DECODE_THD_EXIT:
                return -1;
            default:
                fprintf(stderr, "warning, result_get: %d\n", ret);
                continue;
        }

        sprintf(file_name, "./2_%d_%hu-%hu_%u.yuv",
            n, p_res->stride, p_res->height, i);
        file = fopen(file_name, "wb");
        if (file) {
            /* nv21 */
            fwrite(p_res->buf, 1,
                (p_res->stride * p_res->height) * 3 / 2, file);
            fclose(file);
        } else {
            fprintf(stderr, "warning, fopen failed\n");
            return -1;
        }
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    pollux_decode_t *p_pollux = NULL;
    int ret = pollux_decode_init(&p_pollux);
    if (ret) return ret;

    pollux_decode_yuv_t yuv = {0};
    pollux_decode_param_t param = {0};
    pollux_decode_result_t *p_res = NULL;

    yuv.fmt = POLLUX_FMT_NV21;
    yuv.height = 720;
    yuv.width = 1280;
    yuv.alignment = 1;
    param.fps = 64;
    param.p_file = video_1;
    param.is_loop = IS_LOOP;
    memcpy(&(param.yuv), &yuv, sizeof(pollux_decode_yuv_t));
    ret = p_pollux->param_set(p_pollux, &param);
    if (ret) {
        fprintf(stderr, "error, param_set: %d\n", ret);
        goto label_decode_deinit; \
    }
    ret = pollux_decode_result_alloc(p_pollux, &p_res);
    if(ret) {
        goto label_pollux_release;
    }

    (void)i_file_write(p_pollux, p_res);
    pollux_decode_result_free(p_res);

label_pollux_release:
    p_pollux->release(p_pollux);

label_decode_deinit:
    pollux_decode_deinit(p_pollux);

    return ret;
}
