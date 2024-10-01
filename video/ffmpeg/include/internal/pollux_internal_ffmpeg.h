#ifndef __POLLUX_INTERNAL_FFMPEG_H__
#define __POLLUX_INTERNAL_FFMPEG_H__

#include "sirius_attributes.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"

#include <limits.h>

typedef struct {
    /* format context */
    AVFormatContext *fmt_ctx;

    /* codec context */
    AVCodecContext *codec_ctx;

    /* stream index, such as video, audio, etc  */
    unsigned int stream_index;

    /* sws context */
    struct SwsContext *sws_ctx;

    /* frame data */
    AVFrame *frame;
    /* frame packet */
    AVPacket *pkt;
} internal_ffmpeg_info_t;

typedef struct {
    /* frames per second */
    unsigned short fps;

    /** 
     * cyclic or not decoding,
     * 1: cyclic decoding; 0: non-cyclic decoding
     */
    unsigned short is_loop;

    /* width */
    unsigned short width;
    /* height */
    unsigned short height;
    /* stride */
    unsigned short stride;

    /* the buffer size alignment for yuv */
    size_t alignment;

    /* format, refer to `enum AVPixelFormat` */
    enum AVPixelFormat fmt;

    /* the path of source stream file */
    char src_file_path[PATH_MAX];
} internal_ffmpeg_param_t;

hide_symbol void
internal_ffmpeg_deinit(internal_ffmpeg_info_t *p_ffmpeg);

hide_symbol int
internal_ffmpeg_init(const char *p_file,
    enum AVMediaType media_type,
    internal_ffmpeg_info_t *p_ffmpeg);

hide_symbol void
internal_ffmpeg_resource_free(internal_ffmpeg_info_t *p_ffmpeg);

hide_symbol int
internal_ffmpeg_resource_alloc(internal_ffmpeg_param_t *p_m,
    internal_ffmpeg_info_t *p_ffmpeg);

#endif // __POLLUX_INTERNAL_FFMPEG_H__
