#ifndef __POLLUX_INTERNAL_FMT_H__
#define __POLLUX_INTERNAL_FMT_H__

#include "libavcodec/avcodec.h"
#include "pollux_fmt.h"
#include "pollux_decode.h"
#include "pollux_erron.h"
#include "sirius_log.h"
#include "sirius_attributes.h"

#include <stdbool.h>

#define INTERNAL_POLLUX_FMT_SWITCH(fmt) \
do { \
    switch (fmt) { \
        case POLLUX_FMT_444P: F_444P break; \
        case POLLUX_FMT_NV21: F_NV21 break; \
        case POLLUX_FMT_NV12: F_NV12 break; \
        default: \
            SIRIUS_WARN( \
                "unsupported pollux fmt: %d\n", fmt); \
            F_DFT break; \
    } \
} while (0)

#define INTERNAL_FFMPEG_FMT_SWITCH(fmt) \
do { \
    switch (fmt) { \
        case AV_PIX_FMT_YUV444P: F_444P break; \
        case AV_PIX_FMT_NV21: F_NV21 break; \
        case AV_PIX_FMT_NV12: F_NV12 break; \
        default: \
            SIRIUS_WARN( \
                "unsupported ffmpeg fmt: %d\n", fmt); \
            F_DFT break; \
    } \
} while (0)

hide_symbol bool
internal_fmt_convert(pollux_fmt_t src_fmt,
    enum AVPixelFormat *p_dst_fmt);

hide_symbol unsigned int
internal_fmt_size(enum AVPixelFormat fmt,
    int linesize, int height);

hide_symbol int
internal_fmt_img_result(pollux_decode_result_t *p_res,
    const AVFrame *frame_nv21,
    enum AVPixelFormat fmt);

#endif // __POLLUX_INTERNAL_FMT_H__
