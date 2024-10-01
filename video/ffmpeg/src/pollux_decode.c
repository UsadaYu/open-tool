#include "sirius_queue.h"
#include "sirius_common.h"
#include "pollux_erron.h"

#include "./internal/pollux_internal_decode.h"
#include "./internal/pollux_internal_thread.h"
#include "./internal/pollux_internal_fmt.h"
#include "./internal/pollux_internal_ffmpeg.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    /* thread id */
    pthread_t id;

    /* thread state */
    pollux_internal_thd_state_t state;
} i_pollux_thd_t;

typedef struct {
    /* queue handle, free */
    sirius_que_handle h_que_free;
    /* queue handle, result */
    sirius_que_handle h_que_res;
    /* av_frame cache address */
    AVFrame *p_frame_nv21[INTERNAL_FRAME_NR];

    /* format parameter */
    internal_ffmpeg_param_t param;
    /* parameter setting flag */
    bool param_set_flag;

    /* ffmpeg parameters */
    internal_ffmpeg_info_t ffmpeg;

    /* result mutex */
    pthread_mutex_t mtx;

    /* information of the decode thread */
    i_pollux_thd_t thd;
} i_pollux_t;

static int
i_stream_decode_thd(void *args)
{
    i_pollux_t *p_g = (i_pollux_t *)args;
    internal_ffmpeg_param_t *p_m = &(p_g->param);
    internal_ffmpeg_info_t *p_ffmpeg = &(p_g->ffmpeg);
    AVFormatContext *fmt_ctx = p_ffmpeg->fmt_ctx;
    AVCodecContext *codec_ctx = p_ffmpeg->codec_ctx;
    struct SwsContext *sws_ctx = p_ffmpeg->sws_ctx;
    AVFrame *frame = p_ffmpeg->frame;
    AVPacket *pkt = p_ffmpeg->pkt;
    i_pollux_thd_t *p_thd = &(p_g->thd);

    const int interval_ms = 1000 / p_m->fps;
    struct timespec start, end;
    long elapsed_ms, sleep_ms;
    AVFrame *avf;
    while (p_thd->state == INTERNAL_THD_STATE_RUNNING) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        if (av_read_frame(fmt_ctx, pkt) == AVERROR_EOF) {
            if (unlikely(!(p_m->is_loop))) goto label_thd_terminal;
            if (avformat_seek_file(fmt_ctx, p_ffmpeg->stream_index,
                    0, 0, 0, AVSEEK_FLAG_BACKWARD) < 0) {
                SIRIUS_ERROR("avformat_seek_file\n");
                goto label_thd_terminal;
            }
            SIRIUS_INFO("video loop\n");
            goto label_continue;
        }

        if ((pkt->stream_index != p_ffmpeg->stream_index) ||
            /* send packet to the decoder */
            avcodec_send_packet(codec_ctx, pkt))
            goto label_continue;

        if (sirius_que_get(
            p_g->h_que_free, (size_t *)&avf, 1000) || !(avf))
            goto label_continue;
        if (likely(avcodec_receive_frame(codec_ctx, frame) == 0)) {
            avf->height = sws_scale(
                sws_ctx, (const uint8_t * const *)(frame->data),
                frame->linesize, 0, codec_ctx->height,
                avf->data, avf->linesize);
            avf->width = p_m->width;
            avf->format = p_m->fmt;
            sirius_que_put(p_g->h_que_res, (size_t)avf, 1000);
        } else {
            sirius_que_put(p_g->h_que_free, (size_t)avf, 1000);
        }

label_continue:
        av_packet_unref(pkt);
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 + 
            (end.tv_nsec - start.tv_nsec) / 1000000;
        sleep_ms = interval_ms - elapsed_ms;
        if (sleep_ms > 0) usleep(sleep_ms * 1000);
    }

    p_thd->state = INTERNAL_THD_STATE_EXITED;
    return POLLUX_OK;

label_thd_terminal:
    p_thd->state = INTERNAL_THD_STATE_TERMINATION;
    return POLLUX_ERR_DECODE_THD_EXIT;
}

static void
i_frame_cache_free(i_pollux_t *p_g)
{
    for (unsigned int i = 0; i < INTERNAL_FRAME_NR; i++) {
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
i_frame_cache_alloc(i_pollux_t *p_g)
{
    sirius_que_cr_t cr = {0};
    cr.elem_nr = INTERNAL_FRAME_NR;
    cr.que_type = SIRIUS_QUE_TYPE_MTX;
#define QUE_CR(q) \
    if (sirius_que_cr(&cr, &(q))) { \
        SIRIUS_ERROR("sirius_que_cr\n"); \
        goto label_frame_cache_free; \
    }
    QUE_CR(p_g->h_que_free);
    QUE_CR(p_g->h_que_res);

    for (unsigned int i = 0; i < INTERNAL_FRAME_NR; i++) {
        p_g->p_frame_nv21[i] = av_frame_alloc();
        if (!(p_g->p_frame_nv21[i])) {
            SIRIUS_ERROR("av_frame_alloc\n");
            goto label_frame_cache_free;
        }
    }

#undef QUE_CR
    return POLLUX_OK;

label_frame_cache_free:
    i_frame_cache_free(p_g);
#undef QUE_CR
    return POLLUX_ERR_RESOURCE_REQUEST;
}

static void
i_frame_data_free(i_pollux_t *p_g)
{
    for (unsigned int i = 0; i < INTERNAL_FRAME_NR; i++) {
        if (p_g->p_frame_nv21[i]->data[0]) {
            av_freep(&(p_g->p_frame_nv21[i]->data[0]));
        }
    }

    (void)sirius_que_reset(p_g->h_que_free);
    (void)sirius_que_reset(p_g->h_que_res);
}

static int
i_frame_data_alloc(i_pollux_t *p_g)
{
    i_frame_data_free(p_g);

    AVFrame *p_f;
    internal_ffmpeg_param_t *p_pm = &(p_g->param);
    for (unsigned int i = 0; i < INTERNAL_FRAME_NR; i++) {
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

    p_pm->stride = p_g->p_frame_nv21[0]->linesize[0];

    return POLLUX_OK;

label_frame_data_free:
    i_frame_data_free(p_g);
    return POLLUX_ERR;
}

static void
i_decoder_deinit(i_pollux_t *p_g)
{
    i_pollux_thd_t *p_thd = &(p_g->thd);
    int count = 20;
    switch (p_thd->state) {
        case INTERNAL_THD_STATE_INVALID:
            goto label_ffmpeg_free;
        case INTERNAL_THD_STATE_RUNNING:
            p_thd->state = INTERNAL_THD_STATE_EXITING;
            break;
        default:
            goto label_thd_join;
    }
    while((count--) &&
        (p_thd->state != INTERNAL_THD_STATE_EXITED)) {
        usleep(200 * 1000);
    }
    if (p_thd->state != INTERNAL_THD_STATE_EXITED) {
        SIRIUS_WARN("thread[%lu] cancel\n", p_thd->id);
        pthread_cancel(p_thd->id);
    }

label_thd_join:
    pthread_join(p_thd->id, NULL);

label_ffmpeg_free:
    internal_ffmpeg_resource_free(&(p_g->ffmpeg));

    internal_ffmpeg_deinit(&(p_g->ffmpeg));
}

static int
i_decoder_init(i_pollux_t *p_g)
{
    internal_ffmpeg_info_t *p_ffmpeg = &(p_g->ffmpeg);
    internal_ffmpeg_param_t *p_param = &(p_g->param);
    int ret = internal_ffmpeg_init(p_param->src_file_path,
        AVMEDIA_TYPE_VIDEO, p_ffmpeg);
    if (ret) return ret;

    ret = internal_ffmpeg_resource_alloc(p_param, p_ffmpeg);
    if (ret) goto label_ffmpeg_deinit;

    p_g->thd.state = INTERNAL_THD_STATE_RUNNING;
    ret = pthread_create(&(p_g->thd.id), NULL,
        (void *)i_stream_decode_thd, (void *)p_g);
    if (ret) {
        SIRIUS_ERROR("pthread_create: %d\n", ret);
        goto label_ffmpeg_resource_free;
    }

    return POLLUX_OK;

label_ffmpeg_resource_free:
    internal_ffmpeg_resource_free(p_ffmpeg);

label_ffmpeg_deinit:
    internal_ffmpeg_deinit(p_ffmpeg);

    return POLLUX_ERR;
}

static int
i_decode_param_set(pollux_decode_t *thiz,
    const pollux_decode_param_t *p_param)
{
    if (!(thiz)) return POLLUX_ERR_INVALID_ENTRY;
    i_pollux_t *p_g = (i_pollux_t *)(thiz->priv_data);
    if (!(p_g)) return POLLUX_ERR_NULL_POINTER;

    int ret = POLLUX_OK;
    pthread_mutex_lock(&(p_g->mtx));
    if (p_g->param_set_flag) {
        i_decoder_deinit(p_g);
        p_g->param_set_flag = false;
    }

    internal_ffmpeg_param_t *p_pm = &(p_g->param);
    if (!(internal_fmt_convert(
        p_param->yuv.fmt, &(p_pm->fmt)))) {
        ret = POLLUX_ERR_INVALID_PARAMETER;
        goto label_mtx_unlock;
    }

    p_pm->fps = p_param->fps;
    p_pm->is_loop = p_param->is_loop;
    p_pm->width = p_param->yuv.width;
    p_pm->height = p_param->yuv.height;
    p_pm->alignment = p_param->yuv.alignment;
    strncpy(p_pm->src_file_path, p_param->p_file,
        sizeof(p_pm->src_file_path) - 1);

    ret = i_frame_data_alloc(p_g);
    if (ret) goto label_mtx_unlock;

    ret = i_decoder_init(p_g);
    if (ret) {
        i_frame_data_free(p_g);
        goto label_mtx_unlock;

    }
    p_g->param_set_flag = true;

label_mtx_unlock:
    pthread_mutex_unlock(&(p_g->mtx));

    return ret;
}

static int
i_decode_release(pollux_decode_t *thiz)
{
    if (!(thiz)) return POLLUX_ERR_INVALID_ENTRY;
    i_pollux_t *p_g = (i_pollux_t *)(thiz->priv_data);
    if (!(p_g)) return POLLUX_ERR_NULL_POINTER;

    pthread_mutex_lock(&(p_g->mtx));

    if (!(p_g->param_set_flag)) {
        goto label_mtx_unlock;
    }

    i_decoder_deinit(p_g);

    i_frame_data_free(p_g);

label_mtx_unlock:
    pthread_mutex_unlock(&(p_g->mtx));
    return POLLUX_OK;
}

static int
i_decode_result_get(pollux_decode_t *thiz,
    pollux_decode_result_t *p_res)
{
    if (!(thiz)) return POLLUX_ERR_INVALID_ENTRY;
    i_pollux_t *p_g = (i_pollux_t *)(thiz->priv_data);
    if (!(p_g)) return POLLUX_ERR_NULL_POINTER;
    if (!(p_res) || !(p_res->buf))
        return POLLUX_ERR_NULL_POINTER;

    int ret = POLLUX_OK;
    pthread_mutex_lock(&(p_g->mtx));
    if (!(p_g->param_set_flag)) {
        ret = POLLUX_ERR_NOT_INIT;
        goto label_mtx_unlock;
    }

    switch (p_g->thd.state) {
        case INTERNAL_THD_STATE_TERMINATION:
            SIRIUS_DEBG(
                "the decode thread has terminated\n");
            ret = (p_g->param.is_loop) ?
                POLLUX_ERR_DECODE_THD_EXIT :
                POLLUX_ERR_FILE_END;
            goto label_mtx_unlock;
        default: break;
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

    ret = internal_fmt_img_result(
        p_res, frame_nv21, frame_nv21->format);

    if (sirius_que_put(p_g->h_que_free, (size_t)frame_nv21,
        SIRIUS_QUE_TIMEOUT_NONE)) {
        SIRIUS_WARN("sirius_que_put\n");
    }

label_mtx_unlock:
    pthread_mutex_unlock(&(p_g->mtx));
    return ret;
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

int
pollux_decode_result_alloc(pollux_decode_t *p_handle,
    pollux_decode_result_t **pp_ressult)
{
    if (!(pp_ressult)) return POLLUX_ERR_INVALID_ENTRY;
    if (!(p_handle)) return POLLUX_ERR_INVALID_ENTRY;
    i_pollux_t *p_g = (i_pollux_t *)(p_handle->priv_data);
    if (!(p_g)) return POLLUX_ERR_NULL_POINTER;

    pthread_mutex_lock(&(p_g->mtx));
    if (!(p_g->param_set_flag)) {
        SIRIUS_WARN("no valid parameter is configured\n");
        pthread_mutex_unlock(&(p_g->mtx));
        return POLLUX_ERR_NOT_INIT;
    }
    pthread_mutex_unlock(&(p_g->mtx));

    internal_ffmpeg_param_t *p_pm = &(p_g->param);
    unsigned int buf_size =
        internal_fmt_size(p_pm->fmt, p_pm->stride, p_pm->height);
    if (buf_size == 0) {
        return POLLUX_ERR_INVALID_PARAMETER;
    }

    pollux_decode_result_t *p_res = (pollux_decode_result_t *)
        calloc(1, sizeof(pollux_decode_result_t));
    if (!(p_res)) {
        SIRIUS_ERROR("calloc\n");
        return POLLUX_ERR_MEMORY_ALLOC;
    }

    p_res->buf = (unsigned char *)calloc(1, buf_size);
    if (!(p_res->buf)) {
        SIRIUS_ERROR("calloc\n");
        free(p_res);
        return POLLUX_ERR_MEMORY_ALLOC;
    } else {
        p_res->stride = p_pm->stride;
    }

    *pp_ressult = p_res;

    return POLLUX_OK;
}

int
pollux_decode_deinit(pollux_decode_t *p_handle)
{
    if (!(p_handle))  return POLLUX_ERR_INVALID_ENTRY;
    i_pollux_t *p_g = (i_pollux_t *)(p_handle->priv_data);
    if (!(p_g)) return POLLUX_ERR_NULL_POINTER;

    pthread_mutex_destroy(&(p_g->mtx));

    i_frame_cache_free(p_g);

    free(p_g);
    p_g = NULL;

    free(p_handle);

    sirius_deinit();
    return POLLUX_OK;
}

int
pollux_decode_init(pollux_decode_t **pp_handle)
{
    sirius_init_t cr = {0};
    cr.log_lv = SIRIUS_LOG_LV_INFO;
    cr.p_pipe = "/var/tmp/log_pipe";
    if (sirius_init(&cr)) return POLLUX_ERR;

    int ret = POLLUX_OK;
    pollux_decode_t *p_h =
        (pollux_decode_t *)calloc(1, sizeof(pollux_decode_t));
    if (!(p_h)) {
        SIRIUS_ERROR("calloc\n");
        ret = POLLUX_ERR_MEMORY_ALLOC;
        goto label_sirius_deinit;
    }

    i_pollux_t *p_g = (i_pollux_t *)calloc(1, sizeof(i_pollux_t));
    if (!(p_g)) {
        SIRIUS_ERROR("calloc\n");
        ret = POLLUX_ERR_MEMORY_ALLOC;
        goto label_handle_free;
    }

    ret = i_frame_cache_alloc(p_g);
    if(ret) goto label_gh_free;

    pthread_mutex_init(&(p_g->mtx), NULL);

    p_h->priv_data = (void *)p_g;
    p_h->param_set = i_decode_param_set;
    p_h->release = i_decode_release;
    p_h->result_get = i_decode_result_get;

    *pp_handle = p_h;
    return POLLUX_OK;

label_gh_free:
    free(p_g);

label_handle_free:
    free(p_h);

label_sirius_deinit:
    sirius_deinit();

    return ret;
}
