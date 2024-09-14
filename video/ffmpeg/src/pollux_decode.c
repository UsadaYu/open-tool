#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "sirius_queue.h"
#include "sirius_log.h"
#include "pollux_decode.h"
#include "pollux_erron.h"

#include "./internal/pollux_internal_decode.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <limits.h>

#define I_FRAME_NR (64)

typedef enum {
    /* invalid state */
    I_THD_STATE_INVALID = 0,

    /* thread exitin */
    I_THD_STATE_EXITING,
    /* thread exited */
    I_THD_STATE_EXITED,
    /* thread is running */
    I_THD_STATE_RUNNING,

    /* thread self-termination */
    I_THD_STATE_TERMINATION,

    I_THD_STATE_MAX,
} i_thd_state_t;

typedef enum {
    /* default state, unallowed */
    I_RES_ALLOW_DFT,

    /* the status after the parameter is set, unallowed */
    I_RES_ALLOW_PARAM_SET,

    /* allowable */
    I_RES_ALLOW_OK,
} i_res_allow_t;

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

    /* the buffer size alignment for yuv */
    size_t alignment;

    /* format, refer to `enum AVPixelFormat` */
    enum AVPixelFormat fmt;

    /* the path of source stream file */
    char src_file_path[PATH_MAX];
} i_pollux_param_t;

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
} i_pollux_ffmpeg_info_t;

typedef struct {
    /* thread id */
    pthread_t id;

    /* thread mutex */
    pthread_mutex_t mtx;

    /* thread state */
    i_thd_state_t state;
} i_pollux_thd_t;

typedef struct {
    /* queue handle, free */
    sirius_que_handle h_que_free;
    /* queue handle, result */
    sirius_que_handle h_que_res;
    /* av_frame cache address */
    AVFrame *p_frame_nv21[I_FRAME_NR];

    /* format parameter */
    i_pollux_param_t param;
    /* parameter setting flag */
    bool param_set_flag;

    /* ffmpeg parameters */
    i_pollux_ffmpeg_info_t ffmpeg;

    /**
     * whether to allow the result to be obtained,
     * refer to `i_res_allow_t`
     */
    i_res_allow_t res_allow;
    /* result mutex */
    pthread_mutex_t mtx;

    /* information of the decode thread */
    i_pollux_thd_t thd;
} i_pollux_t;

static void
i_decode_fmt_ctx_del(AVFormatContext *fmt_ctx)
{
    avformat_close_input(&fmt_ctx);

    avformat_free_context(fmt_ctx);
}

static int
i_decode_fmt_ctx_cr(AVFormatContext **fmt_ctx,
    const char *p_file)
{
    /**
     * allocate the `av` context,
     * which is used to store audio and video information
     */
    if (!(*fmt_ctx = avformat_alloc_context())) {
        SIRIUS_ERROR("avformat_alloc_context\n");
        return POLLUX_ERR_RESOURCE_REQUEST;
    }

    int ret;
    /**
     * open the input file, and request the appropriate resource
     * for the members in the `fmt_ctx` based on the file
     */
    ret = avformat_open_input(fmt_ctx, p_file, NULL, NULL);
    if (ret) {
        SIRIUS_ERROR("avformat_open_input: [%d]\n", ret);
        goto label_fmt_ctx_free;
    }

    /* read information from input file, and fill them into `fmt_ctx` */
    ret = avformat_find_stream_info(*fmt_ctx, NULL);
    if (ret < 0) {
        SIRIUS_ERROR("avformat_find_stream_info: [%d]\n", ret);
        goto label_input_close;
    }

    return POLLUX_OK;

label_input_close:
    avformat_close_input(fmt_ctx);

label_fmt_ctx_free:
    avformat_free_context(*fmt_ctx);
    *fmt_ctx = NULL;

    return POLLUX_ERR;
}

static void
i_decode_decoder_del(AVCodecContext *codec_ctx)
{
    /* this function will close `codec` and free `codec` */
    avcodec_free_context(&codec_ctx);
}

static AVCodecContext *
i_decode_decoder_cr(AVCodecParameters *codec_param)
{
    /**
     * `codec_id` stands for the decoder used,
     * such as H.264, HEVC, etc
     */
    SIRIUS_DEBG("codec_id: %d\n", codec_param->codec_id);
    /* find a decoder based on `codec_id` */
    const AVCodec *codec =
        avcodec_find_decoder(codec_param->codec_id);
    if (!(codec)) {
        SIRIUS_ERROR("avcodec_find_decoder\n");
        return NULL;
    }

    /* allocate decoder context based on `codec` information */
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!(codec)) {
        SIRIUS_ERROR("avcodec_alloc_context3\n");
        return NULL;
    }

    int ret;
    /* copy the parameters `codec_param` to `codec_ctx` */
    ret = avcodec_parameters_to_context(codec_ctx, codec_param);
    if (ret < 0) {
        SIRIUS_ERROR(
            "avcodec_parameters_to_context: [%d]\n", ret);
        goto label_codec_free;
    }

    /**
     * open the decoder and
     * associate the decoder with the codec_ctx
     */
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        SIRIUS_ERROR("avcodec_open2: [%d]\n", ret);
        goto label_codec_free;
    }

    SIRIUS_DEBG("[src_wd: %d], [src_hgt: %d]\n",
        codec_ctx->width, codec_ctx->height);
    return codec_ctx;

label_codec_free:
    avcodec_free_context(&codec_ctx);

    return NULL;
}

static int
i_decode_stream_decode_thd(void *args)
{
    i_pollux_t *p_g = (i_pollux_t *)args;
    i_pollux_param_t *p_m = &(p_g->param);
    i_pollux_ffmpeg_info_t *p_ffmpeg = &(p_g->ffmpeg);
    AVFormatContext *fmt_ctx = p_ffmpeg->fmt_ctx;
    AVCodecContext *codec_ctx = p_ffmpeg->codec_ctx;
    struct SwsContext *sws_ctx = p_ffmpeg->sws_ctx;
    AVFrame *frame = p_ffmpeg->frame;
    AVPacket *pkt = p_ffmpeg->pkt;
    i_pollux_thd_t *p_thd = &(p_g->thd);

    const int interval_ms = 1000 / p_m->fps;
    struct timespec start, end;
    long elapsed_ms, sleep_ms;
    int ret;
    AVFrame *frame_nv21;
#define THD_TMN \
    pthread_mutex_unlock(&(p_thd->mtx)); \
    p_thd->state = I_THD_STATE_TERMINATION; \
    return POLLUX_ERR_DECODE_THD_EXIT;

    while (p_thd->state == I_THD_STATE_RUNNING) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        pthread_mutex_lock(&(p_thd->mtx));
        /* read a frame of data from the `fmt_ctx` into `pkt` */
        ret = av_read_frame(fmt_ctx, pkt);
        if (ret == AVERROR_EOF) {
            if (p_m->is_loop) {
                if (avformat_seek_file(fmt_ctx, p_ffmpeg->stream_index,
                        0, 0, 0, AVSEEK_FLAG_BACKWARD) < 0) {
                    SIRIUS_ERROR("avformat_seek_file\n");
                    THD_TMN
                } else {
                    SIRIUS_INFO("video loop\n");
                }
                goto label_continue;
            } else { THD_TMN }
        }
        if (pkt->stream_index != p_ffmpeg->stream_index)
            goto label_continue;

        /* send packet to the decoder */
        ret = avcodec_send_packet(codec_ctx, pkt);
        av_packet_unref(pkt);
        if (ret) goto label_continue;

        ret = sirius_que_get(p_g->h_que_free, (size_t *)&frame_nv21, 1000);
        if (ret || !(frame_nv21)) goto label_continue;
        if (avcodec_receive_frame(codec_ctx, frame) == 0) {
            frame_nv21->height = sws_scale(
                sws_ctx, (const uint8_t * const *)(frame->data),
                frame->linesize, 0, codec_ctx->height,
                frame_nv21->data, frame_nv21->linesize);
            frame_nv21->width = p_m->width;
            frame_nv21->format = p_m->fmt;
            sirius_que_put(p_g->h_que_res, (size_t)frame_nv21, 1000);
        } else {
            sirius_que_put(p_g->h_que_free, (size_t)frame_nv21, 1000);
        }

label_continue:
        pthread_mutex_unlock(&(p_thd->mtx));
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 + 
            (end.tv_nsec - start.tv_nsec) / 1000000;
        sleep_ms = interval_ms - elapsed_ms;
        if (sleep_ms > 0) usleep(sleep_ms * 1000);
    }

    p_thd->state = I_THD_STATE_EXITED;
    return POLLUX_OK;
}

void
i_decode_ffmpeg_deinit(i_pollux_ffmpeg_info_t *p_ffmpeg)
{
    AVFormatContext *fmt_ctx = p_ffmpeg->fmt_ctx;

    i_decode_decoder_del(p_ffmpeg->codec_ctx);

    i_decode_fmt_ctx_del(fmt_ctx);
}

int
i_decode_ffmpeg_init(const i_pollux_param_t *p_m,
    i_pollux_ffmpeg_info_t *p_ffmpeg)
{
    if(i_decode_fmt_ctx_cr(&(p_ffmpeg->fmt_ctx),
        p_m->src_file_path)) {
        return POLLUX_ERR;
    }
    AVFormatContext *fmt_ctx = p_ffmpeg->fmt_ctx;

    if (pollux_internal_fmt_stream_get(
        &(p_ffmpeg->stream_index), fmt_ctx, AVMEDIA_TYPE_VIDEO)) {
        goto label_fmt_ctx_del;
    }

    p_ffmpeg->codec_ctx = i_decode_decoder_cr(
            fmt_ctx->streams[p_ffmpeg->stream_index]->codecpar);
    if (!(p_ffmpeg->codec_ctx)) {
        goto label_fmt_ctx_del;
    }

    return POLLUX_OK;

label_fmt_ctx_del:
    i_decode_fmt_ctx_del(fmt_ctx);

    return POLLUX_ERR;
}

static void
i_decode_ffmpeg_resource_free(i_pollux_ffmpeg_info_t *p_ffmpeg)
{
    av_packet_free(&(p_ffmpeg->pkt));

    av_frame_free(&(p_ffmpeg->frame));

    sws_freeContext(p_ffmpeg->sws_ctx);
}

static int
i_decode_ffmpeg_resource_alloc(i_pollux_param_t *p_m,
    i_pollux_ffmpeg_info_t *p_ffmpeg)
{
    AVCodecContext *codec_ctx = p_ffmpeg->codec_ctx;
    /**
     * allocate resource for `sws_ctx`
     * `sws_ctx` is used for video pixel format conversion
     * and image scaling operations
     */
    p_ffmpeg->sws_ctx = sws_getContext(
        codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
        p_m->width, p_m->height, p_m->fmt,
        SWS_BILINEAR, NULL, NULL, NULL);
    if (!(p_ffmpeg->sws_ctx)) {
        SIRIUS_ERROR("sws_getContext\n");
        return POLLUX_ERR;
    }

    p_ffmpeg->frame = av_frame_alloc();
    if(!(p_ffmpeg->frame)) {
        SIRIUS_ERROR("av_frame_alloc\n");
        goto label_sws_ctx_free;
    }

    p_ffmpeg->pkt = av_packet_alloc();
    if (!(p_ffmpeg->pkt)) {
        SIRIUS_ERROR("av_packet_alloc\n");
        goto label_frame_free;
    }

    return POLLUX_OK;

label_frame_free:
    av_frame_free(&(p_ffmpeg->frame));

label_sws_ctx_free:
    sws_freeContext(p_ffmpeg->sws_ctx);

    return POLLUX_ERR;
}

static void
i_decode_frame_cache_free(i_pollux_t *p_g)
{
    for (unsigned int i = 0; i < I_FRAME_NR; i++) {
        if (p_g->p_frame_nv21[i]) {
            av_frame_free(&(p_g->p_frame_nv21[i]));
            p_g->p_frame_nv21[i] = NULL;
        }
    }

#define QUE_DEL(q) \
    if (q) { \
        if (sirius_que_del(q)) { \
            SIRIUS_ERROR("sirius_que_del\n"); \
        } else { \
            q = NULL; \
        } \
    }
    QUE_DEL(p_g->h_que_res);
    QUE_DEL(p_g->h_que_free);
#undef QUE_DEL
}

static int
i_decode_frame_cache_alloc(i_pollux_t *p_g)
{
    sirius_que_cr_t cr = {0};
    cr.elem_nr = I_FRAME_NR;
    cr.que_type = SIRIUS_QUE_TYPE_MTX;
#define QUE_CR(q) \
    if (sirius_que_cr(&cr, &(q))) { \
        SIRIUS_ERROR("sirius_que_cr\n"); \
        goto label_frame_cache_free; \
    }
    QUE_CR(p_g->h_que_free);
    QUE_CR(p_g->h_que_res);

    for (unsigned int i = 0; i < I_FRAME_NR; i++) {
        p_g->p_frame_nv21[i] = av_frame_alloc();
        if (!(p_g->p_frame_nv21[i])) {
            SIRIUS_ERROR("av_frame_alloc\n");
            goto label_frame_cache_free;
        }
    }

#undef QUE_CR
    return POLLUX_OK;

label_frame_cache_free:
    i_decode_frame_cache_free(p_g);
#undef QUE_CR
    return POLLUX_ERR_RESOURCE_REQUEST;
}

static void
i_decode_frame_data_free(i_pollux_t *p_g)
{
    for (unsigned int i = 0; i < I_FRAME_NR; i++) {
        if (p_g->p_frame_nv21[i]->data[0]) {
            av_freep(&(p_g->p_frame_nv21[i]->data[0]));
        }
    }

    (void)sirius_que_reset(p_g->h_que_free);
    (void)sirius_que_reset(p_g->h_que_res);
}

static int
i_decode_frame_data_alloc(i_pollux_t *p_g)
{
    i_decode_frame_data_free(p_g);

    AVFrame *p_f;
    i_pollux_param_t *p_pm = &(p_g->param);
    for (unsigned int i = 0; i < I_FRAME_NR; i++) {
        p_f = p_g->p_frame_nv21[i];
        if (0 > av_image_alloc(
            p_f->data, p_f->linesize,
            p_pm->width, p_pm->height, p_pm->fmt, p_pm->alignment)) {
            SIRIUS_ERROR("av_image_alloc\n");
            goto label_frame_data_free;
        }

        if (sirius_que_put(
            p_g->h_que_free, (size_t)p_f, SIRIUS_QUE_TIMEOUT_NONE)) {
            SIRIUS_ERROR("sirius_que_put\n");
            goto label_frame_data_free;
        }
    }

    return POLLUX_OK;

label_frame_data_free:
    i_decode_frame_data_free(p_g);
    return POLLUX_ERR;
}

static void
i_decode_result_nv21(pollux_decode_result_t *p_res,
    const AVFrame *frame_nv21)
{
    p_res->fmt = POLLUX_DECODE_FMT_NV21;

    unsigned int y_size =
        frame_nv21->linesize[0] * frame_nv21->height;
    unsigned int vu_size = y_size >> 1;

    memcpy(p_res->buf, frame_nv21->data[0], y_size);
    memcpy(p_res->buf + y_size, frame_nv21->data[1], vu_size);
}

static void
i_decode_decoder_deinit(i_pollux_t *p_g)
{
    i_pollux_thd_t *p_thd = &(p_g->thd);
    int count = 20;
    switch (p_thd->state) {
        case I_THD_STATE_INVALID:
            goto label_ffmpeg_free;
        case I_THD_STATE_RUNNING:
            p_thd->state = I_THD_STATE_EXITING;
            break;
        default:
            goto label_thd_join;
    }
    while((count--) && (p_thd->state != I_THD_STATE_EXITED)) {
        usleep(200 * 1000);
    }
    if (p_thd->state != I_THD_STATE_EXITED) {
        SIRIUS_WARN("thread[%lu] cancel\n", p_thd->id);
        pthread_cancel(p_thd->id);
    }

label_thd_join:
    pthread_join(p_thd->id, NULL);

label_ffmpeg_free:
    i_decode_ffmpeg_resource_free(&(p_g->ffmpeg));

    i_decode_ffmpeg_deinit(&(p_g->ffmpeg));
}

static int
i_decode_decoder_init(i_pollux_t *p_g)
{
    int ret = i_decode_ffmpeg_init(
        &(p_g->param), &(p_g->ffmpeg));
    if (ret) return ret;
    i_pollux_ffmpeg_info_t *p_ffmpeg = &(p_g->ffmpeg);

    ret = i_decode_ffmpeg_resource_alloc(
        &(p_g->param), &(p_g->ffmpeg));
    if (ret) goto label_ffmpeg_deinit;

    p_g->thd.state = I_THD_STATE_RUNNING;
    ret = pthread_create(&(p_g->thd.id), NULL,
        (void *)i_decode_stream_decode_thd, (void *)p_g);
    if (ret) goto label_ffmpeg_resource_free;

    return POLLUX_OK;

label_ffmpeg_resource_free:
    i_decode_ffmpeg_resource_free(p_ffmpeg);

label_ffmpeg_deinit:
    i_decode_ffmpeg_deinit(p_ffmpeg);

    return POLLUX_ERR;
}

static int
i_pollux_decode_param_set(pollux_decode_t *thiz,
    const pollux_decode_param_t *p_param)
{
    if (!(thiz)) return POLLUX_ERR_INVALID_ENTRY;
    i_pollux_t *p_g = (i_pollux_t *)(thiz->priv_data);
    if (!(p_g)) return POLLUX_ERR_NULL_POINTER;

    bool realloc_flag = false;
#define RE_CHECK(src, dst) \
    if (dst != src) { \
        dst = src; \
        realloc_flag = true; \
    }

    int ret = POLLUX_OK;
    pthread_mutex_lock(&(p_g->mtx));
    p_g->res_allow = I_RES_ALLOW_PARAM_SET;
    pthread_mutex_lock(&(p_g->thd.mtx));
    if (p_g->param_set_flag) {
        i_decode_decoder_deinit(p_g);
        p_g->param_set_flag = false;
    }
    i_pollux_param_t *p_pm = &(p_g->param);

#define NV21 \
    RE_CHECK(AV_PIX_FMT_NV21, p_pm->fmt) \
    break;
#define DFT \
    ret = POLLUX_ERR_INVALID_PARAMETER; \
    goto label_return;
    POLLUX_INTERNAL_FMT_SWITCH(p_param->yuv.fmt);

    p_pm->fps = p_param->fps;
    p_pm->is_loop = p_param->is_loop;
    RE_CHECK(p_param->yuv.width, p_pm->width)
    RE_CHECK(p_param->yuv.height, p_pm->height)
    RE_CHECK(p_param->yuv.alignment, p_pm->alignment)
    strncpy(p_pm->src_file_path, p_param->p_file,
        sizeof(p_pm->src_file_path) - 1);

    if (realloc_flag) {
        ret = i_decode_frame_data_alloc(p_g);
        if (ret) goto label_return;
    }

    ret = i_decode_decoder_init(p_g);
    if (ret) goto label_return;
    p_g->param_set_flag = true;

label_return:
    pthread_mutex_unlock(&(p_g->mtx));
    pthread_mutex_unlock(&(p_g->thd.mtx));
#undef DFT
#undef NV21
#undef RE_CHECK
    return ret;
}

static int
i_pollux_decode_release(pollux_decode_t *thiz)
{
    if (!(thiz)) return POLLUX_ERR_INVALID_ENTRY;
    i_pollux_t *p_g = (i_pollux_t *)(thiz->priv_data);
    if (!(p_g)) return POLLUX_ERR_NULL_POINTER;

    i_decode_decoder_deinit(p_g);

    i_decode_frame_data_free(p_g);

    return POLLUX_OK;
}

/**
 * here, need to make sure that if the current function and
 * the `i_pollux_decode_param_set` function are in different
 * threads, this function can exit smoothly, so the parameter
 * `res_allow` is used to do just that
 */
static int
i_pollux_decode_result_get(pollux_decode_t *thiz,
    pollux_decode_result_t *p_res)
{
    if (!(thiz)) return POLLUX_ERR_INVALID_ENTRY;
    i_pollux_t *p_g = (i_pollux_t *)(thiz->priv_data);
    if (!(p_g)) return POLLUX_ERR_NULL_POINTER;

    if (!(p_res) || !(p_res->buf)) return POLLUX_ERR_NULL_POINTER;
    switch (p_g->thd.state) {
        case I_THD_STATE_TERMINATION:
            SIRIUS_DEBG("the decode thread has terminated\n");
            return POLLUX_ERR_DECODE_THD_EXIT;
        default:
            break;
    }

    int ret = POLLUX_OK;
    pthread_mutex_lock(&(p_g->mtx));
    switch (p_g->res_allow) {
        case I_RES_ALLOW_DFT:
            ret = POLLUX_ERR_NOT_INIT;
            goto label_mtx_unlock;
        case I_RES_ALLOW_PARAM_SET:
            SIRIUS_WARN(
                "wait for parameters to be set, don't worry\n");
            p_g->res_allow = I_RES_ALLOW_OK;
            ret = POLLUX_ERR_NOT_INIT;
            goto label_mtx_unlock;
        default:
            break;
    }

    AVFrame *frame_nv21 = NULL;
    ret = sirius_que_get(
        p_g->h_que_res, (size_t *)&frame_nv21, 1000);
    if (ret || !(frame_nv21)) {
        ret = POLLUX_ERR_RESOURCE_REQUEST;
        goto label_mtx_unlock;
    }

    p_res->width = frame_nv21->width;
    p_res->height = frame_nv21->height;
    p_res->stride = frame_nv21->linesize[0];

    switch (frame_nv21->format) {
        case AV_PIX_FMT_NV21:
            i_decode_result_nv21(p_res, frame_nv21);
            break;
        default:
            ret = POLLUX_ERR_INVALID_PARAMETER;
            break;
    }

    if (sirius_que_put(
        p_g->h_que_free, (size_t)frame_nv21,
        SIRIUS_QUE_TIMEOUT_NONE)) {
        SIRIUS_WARN("sirius_que_put\n");
    }

label_mtx_unlock:
    pthread_mutex_unlock(&(p_g->mtx));
    return ret;
}

int
pollux_internal_fmt_stream_get(unsigned int *p_stream,
    AVFormatContext *fmt_ctx,
    enum AVMediaType media_type)
{
    *p_stream = -1;
    for (unsigned int i = 0;
        i < fmt_ctx->nb_streams; i++) {
        if (media_type ==
            fmt_ctx->streams[i]->codecpar->codec_type) {
            *p_stream = i;
            break;
        }
    }
    if (*p_stream == -1) {
        SIRIUS_ERROR(
            "media type [%d] not found\n", media_type);
        return POLLUX_ERR;
    }

    return POLLUX_OK;
}

void
pollux_decode_result_free(pollux_decode_result_t *p_result)
{
    if (p_result) {
        if (p_result->buf) {
            free(p_result->buf);
            p_result->buf = NULL;
        }

        free(p_result);
    }
}

pollux_decode_result_t *
pollux_decode_result_alloc(pollux_decode_yuv_t *p_yuv)
{
    unsigned short wd = p_yuv->width;
    unsigned short hgt = p_yuv->height;
    int align = p_yuv->alignment;
    pollux_decode_fmt_t fmt = p_yuv->fmt;

    int stride_y = (wd + align - 1) & ~(align - 1);
    int stride_uv = (wd + align - 1) & ~(align - 1);

    int size_y = 0, size_uv = 0;
#define NV21 \
    size_y = stride_y * hgt; \
    size_uv = stride_uv * (hgt >> 1); \
    break;
#define DFT \
    goto label_error;
    POLLUX_INTERNAL_FMT_SWITCH(fmt);

    pollux_decode_result_t *p_result = (pollux_decode_result_t *)
        calloc(1, sizeof(pollux_decode_result_t));
    if (!(p_result)) {
        SIRIUS_ERROR("calloc\n");
        goto label_error;
    }

    p_result->buf = (unsigned char *)calloc(1, size_y + size_uv);
    if (!(p_result->buf)) {
        SIRIUS_ERROR("calloc\n");
        goto label_result_free;
    }
    p_result->stride = stride_y;

    return p_result;

label_result_free:
    free(p_result);

label_error:
#undef DFT
#undef NV21
    return NULL;
}

int
pollux_decode_deinit(pollux_decode_t *p_handle)
{
    if (!(p_handle))  return POLLUX_ERR_INVALID_ENTRY;
    i_pollux_t *p_g = (i_pollux_t *)(p_handle->priv_data);
    if (!(p_g)) return POLLUX_ERR_NULL_POINTER;

    pthread_mutex_destroy(&(p_g->thd.mtx));
    pthread_mutex_destroy(&(p_g->mtx));

    i_decode_frame_cache_free(p_g);

    free(p_g);
    p_g = NULL;

    free(p_handle);

    return POLLUX_OK;
}

int
pollux_decode_init(pollux_decode_t **pp_handle)
{
    pollux_decode_t *p_h =
        (pollux_decode_t *)calloc(1, sizeof(pollux_decode_t));
    if (!(p_h)) {
        SIRIUS_ERROR("calloc\n");
        return POLLUX_ERR_MEMORY_ALLOC;
    }

    i_pollux_t *p_g = (i_pollux_t *)calloc(1, sizeof(i_pollux_t));
    if (!(p_g)) {
        SIRIUS_ERROR("calloc\n");
        goto label_handle_free;
    }

    if(i_decode_frame_cache_alloc(p_g)) {
        goto label_gh_free;
    }

    pthread_mutex_init(&(p_g->mtx), NULL);
    pthread_mutex_init(&(p_g->thd.mtx), NULL);

    p_h->priv_data = (void *)p_g;
    p_h->param_set = i_pollux_decode_param_set;
    p_h->release = i_pollux_decode_release;
    p_h->result_get = i_pollux_decode_result_get;

    *pp_handle = p_h;
    return POLLUX_OK;

label_gh_free:
    free(p_g);

label_handle_free:
    free(p_h);

    return POLLUX_ERR;
}
