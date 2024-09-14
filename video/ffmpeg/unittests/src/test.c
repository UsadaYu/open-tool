#include "sirius_common.h"
#include "sirius_errno.h"
#include "pollux_decode.h"
#include "pollux_erron.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define YUV_NR (512)
#define IS_LOOP (1)

static char *video_1 = "./input1.mp4";
static char *video_2 = "./input4.dav";

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
            case POLLUX_ERR_NOT_INIT:
                continue;
            case POLLUX_ERR_DECODE_THD_EXIT:
                return -1;
            default:
                SIRIUS_WARN("result_get: %d\n", ret);
                continue;
        }
        sprintf(file_name, "./temp/%d_%hu-%hu_%u.yuv",
            n, p_res->stride, p_res->height, i);
        file = fopen(file_name, "wb");
        if (file) {
            /* nv21 */
            fwrite(p_res->buf, 1,
                (p_res->stride * p_res->height) * 3 / 2, file);
            fclose(file);
        } else {
            SIRIUS_WARN("fopen failed\n");
            return -1;
        }
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    sirius_init_t cr = {0};
    cr.log_lv = SIRIUS_LOG_LV_DEBG;
    cr.p_pipe = "/var/tmp/log_pipe";
    if (sirius_init(&cr)) return SIRIUS_ERR;

    pollux_decode_t *p_pollux = NULL;
    int ret = pollux_decode_init(&p_pollux);
    if (ret) goto label_sirius_deinit;

    pollux_decode_yuv_t yuv = {0};
    pollux_decode_param_t param = {0};
    pollux_decode_result_t *p_res = NULL;

#define work_flow(ft, hgt, wd, ali, fp, f) \
    yuv.fmt = ft; \
    yuv.height = hgt; \
    yuv.width = wd; \
    yuv.alignment = ali; \
    param.fps = fp; \
    param.p_file = f; \
    param.is_loop = IS_LOOP; \
    memcpy(&(param.yuv), &yuv, \
        sizeof(pollux_decode_yuv_t)); \
    ret = p_pollux->param_set(p_pollux, &param); \
    if (ret) { \
        SIRIUS_ERROR("param_set: %d\n", ret); \
        goto label_decode_deinit; \
    } \
    p_res = pollux_decode_result_alloc(&yuv); \
    if (!(p_res)) { \
        ret = SIRIUS_ERR_RESOURCE_REQUEST; \
        goto label_pollux_release; \
    } \
    if (i_file_write(p_pollux, p_res)) { \
        pollux_decode_result_free(p_res); \
        goto label_pollux_release; \
    } \
    pollux_decode_result_free(p_res);

    work_flow(POLLUX_DECODE_FMT_NV21,
        1440, 2560, 16, 60, video_1)

    work_flow(POLLUX_DECODE_FMT_NV21,
        720, 1280, 4, 30, video_2)

label_pollux_release:
    p_pollux->release(p_pollux);

label_decode_deinit:
    pollux_decode_deinit(p_pollux);

label_sirius_deinit:
    sirius_deinit();

    return ret;
}
