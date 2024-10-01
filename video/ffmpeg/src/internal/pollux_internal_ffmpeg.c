#include "pollux_erron.h"
#include "sirius_log.h"

#include "./internal/pollux_internal_ffmpeg.h"

static inline void
i_fmt_ctx_delete(AVFormatContext *fmt_ctx)
{
    avformat_close_input(&fmt_ctx);

    avformat_free_context(fmt_ctx);
}

static int
i_fmt_ctx_create(AVFormatContext **fmt_ctx,
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

/**
 * @brief find stream index based on `media_type`
 * 
 * @param[out] p_stream: stream index
 * @param[in] fmt_ctx: format context
 * @param[in] media_type: media type, refer `AVMediaType`
 * 
 * @return 0 if OK, error code otherwise
 */
static inline int
i_stream_get(unsigned int *p_stream,
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

static inline void
i_decoder_delete(AVCodecContext *codec_ctx)
{
    /* this function will close `codec` and free `codec` */
    avcodec_free_context(&codec_ctx);
}

static AVCodecContext *
i_decoder_create(AVCodecParameters *codec_param)
{
    /**
     * `codec_id` stands for the decoder used,
     * such as H.264, HEVC, etc
     */
    SIRIUS_INFO("codec_id: %d\n", codec_param->codec_id);
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

    SIRIUS_INFO("[src_wd: %d], [src_hgt: %d]\n",
        codec_ctx->width, codec_ctx->height);
    return codec_ctx;

label_codec_free:
    avcodec_free_context(&codec_ctx);

    return NULL;
}

hide_symbol void
internal_ffmpeg_deinit(internal_ffmpeg_info_t *p_ffmpeg)
{
    AVFormatContext *fmt_ctx = p_ffmpeg->fmt_ctx;

    i_decoder_delete(p_ffmpeg->codec_ctx);

    i_fmt_ctx_delete(fmt_ctx);
}

hide_symbol int
internal_ffmpeg_init(const char *p_file,
    enum AVMediaType media_type,
    internal_ffmpeg_info_t *p_ffmpeg)
{
    if(i_fmt_ctx_create(&(p_ffmpeg->fmt_ctx), p_file)) {
        return POLLUX_ERR;
    }
    AVFormatContext *fmt_ctx = p_ffmpeg->fmt_ctx;

    if (i_stream_get(
        &(p_ffmpeg->stream_index), fmt_ctx, AVMEDIA_TYPE_VIDEO)) {
        goto label_fmt_ctx_del;
    }

    p_ffmpeg->codec_ctx = i_decoder_create(
            fmt_ctx->streams[p_ffmpeg->stream_index]->codecpar);
    if (!(p_ffmpeg->codec_ctx)) {
        goto label_fmt_ctx_del;
    }

    return POLLUX_OK;

label_fmt_ctx_del:
    i_fmt_ctx_delete(fmt_ctx);

    return POLLUX_ERR;
}

hide_symbol void
internal_ffmpeg_resource_free(internal_ffmpeg_info_t *p_ffmpeg)
{
    av_packet_free(&(p_ffmpeg->pkt));

    av_frame_free(&(p_ffmpeg->frame));

    sws_freeContext(p_ffmpeg->sws_ctx);
}

hide_symbol int
internal_ffmpeg_resource_alloc(internal_ffmpeg_param_t *p_m,
    internal_ffmpeg_info_t *p_ffmpeg)
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
