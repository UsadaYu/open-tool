#include "./internal/pollux_internal_fmt.h"

static force_inline void
i_result_444p(pollux_decode_result_t *p_res,
    const AVFrame *p_frame)
{
    p_res->fmt = POLLUX_FMT_444P;

    unsigned int y_size =
        p_frame->linesize[0] * p_frame->height;

    memcpy(p_res->buf, p_frame->data[0], y_size);
    memcpy(p_res->buf + y_size, p_frame->data[1], y_size);
    memcpy(p_res->buf + (y_size << 1), p_frame->data[2], y_size);
}

static force_inline void
i_result_y_uv(pollux_decode_result_t *p_res,
    const AVFrame *p_frame, pollux_fmt_t fmt)
{
    p_res->fmt = fmt;

    unsigned int y_size =
        p_frame->linesize[0] * p_frame->height;
    unsigned int uv_size = y_size >> 1;

    memcpy(p_res->buf, p_frame->data[0], y_size);
    memcpy(p_res->buf + y_size, p_frame->data[1], uv_size);
}

hide_symbol inline bool
internal_fmt_convert(pollux_fmt_t src_fmt,
    enum AVPixelFormat *p_dst_fmt)
{
    bool ret = true;

#define F_444P *p_dst_fmt = AV_PIX_FMT_YUV444P;
#define F_NV21 *p_dst_fmt = AV_PIX_FMT_NV21;
#define F_NV12 *p_dst_fmt = AV_PIX_FMT_NV12;
#define F_DFT ret = false;
    INTERNAL_POLLUX_FMT_SWITCH(src_fmt);

#undef F_DFT
#undef F_NV12
#undef F_NV21
#undef F_444P
    return ret;
}

hide_symbol inline unsigned int
internal_fmt_size(enum AVPixelFormat fmt,
    int linesize, int height)
{
    int buf_size = 0;

#define F_444P buf_size = linesize * height * 3;
#define F_NV21 buf_size = linesize * height * 3 >> 1;
#define F_NV12 buf_size = linesize * height * 3 >> 1;
#define F_DFT buf_size = 0;
    INTERNAL_FFMPEG_FMT_SWITCH(fmt);

#undef F_DFT
#undef F_NV12
#undef F_NV21
#undef F_444P
    return buf_size;
}

hide_symbol inline int
internal_fmt_img_result(pollux_decode_result_t *p_res,
    const AVFrame *p_frame,
    enum AVPixelFormat fmt)
{
    int ret = POLLUX_OK;

#define F_444P i_result_444p(p_res, p_frame);
#define F_NV21 i_result_y_uv(p_res, p_frame, POLLUX_FMT_NV21);
#define F_NV12 i_result_y_uv(p_res, p_frame, POLLUX_FMT_NV12);
#define F_DFT ret = POLLUX_ERR_INVALID_PARAMETER;
    INTERNAL_FFMPEG_FMT_SWITCH(fmt);

#undef F_DFT
#undef F_NV12
#undef F_NV21
#undef F_444P
    return ret;
}
