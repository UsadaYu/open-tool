#ifndef __POLLUX_FMT_H__
#define __POLLUX_FMT_H__

typedef enum {
    POLLUX_FMT_NONE = -1,

    /* yuv 444p */
    POLLUX_FMT_444P,

    /* yuv nv21 */
    POLLUX_FMT_NV21,

    /* yuv nv12 */
    POLLUX_FMT_NV12,

    POLLUX_FMT_MAX,
} pollux_fmt_t;

#endif // __POLLUX_FMT_H__
