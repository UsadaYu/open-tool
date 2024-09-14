#ifndef __POLLUX_INTERNAL_DECODE_H__
#define __POLLUX_INTERNAL_DECODE_H__

#include "libavformat/avformat.h"
#include "sirius_errno.h"
#include "sirius_log.h"
#include "sirius_macro.h"

#include "pollux_decode.h"

#define POLLUX_INTERNAL_FMT_SWITCH(fmt) \
do { \
    switch (fmt) { \
        case POLLUX_DECODE_FMT_NV21: \
            NV21 \
        default: \
            SIRIUS_WARN( \
            "unsupported fmt: %d\n", fmt); \
            DFT \
    } \
} while (0)

/**
 * @brief find stream index based on `media_type`
 * 
 * @param[out] p_stream: stream index
 * @param[in] fmt_ctx: format context
 * @param[in] media_type: media type, refer `AVMediaType`
 * 
 * @return 0 if OK, error code otherwise
 */
int
pollux_internal_fmt_stream_get(unsigned int *p_stream,
    AVFormatContext *fmt_ctx,
    enum AVMediaType media_type);

#endif // __POLLUX_INTERNAL_DECODE_H__
