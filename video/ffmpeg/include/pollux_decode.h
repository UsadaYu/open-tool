#ifndef __POLLUX_DECODE_H__
#define __POLLUX_DECODE_H__

#include "pollux_fmt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* width */
    unsigned short width;
    /* height */
    unsigned short height;

    /* the buffer size alignment for the yuv */
    int alignment;

    /* format, refer to `pollux_fmt_t` */
    pollux_fmt_t fmt;
} pollux_decode_yuv_t;

typedef struct {
    /* frames per second */
    unsigned short fps;

    /** 
     * cyclic or not decoding.
     * 
     * 1: cyclic decoding, after the video decoding
     *  reaches the end, continue to find the video
     *  start loop decoding
     * 
     * 0: non-cyclic decoding, end the decoding thread
     *  directly after the video is decoded to the end,
     *  after the decoding thread calling function
     *  `result_get` returns `POLLUX_ERR_FILE_END`
     */
    unsigned short is_loop;

    /**
     * information of the yuv settings, the function
     * `pollux_decode_result_alloc` will request memory
     *  based on this value
     */
    pollux_decode_yuv_t yuv;

    /* source stream file path */
    const char *p_file;
} pollux_decode_param_t;

typedef struct {
    /* width */
    unsigned short width;
    /* height */
    unsigned short height;
    /**
     * stride, which will be filled after calling the
     * function `pollux_decode_result_alloc`
     */
    unsigned short stride;

    /* format, refer to `pollux_fmt_t` */
    pollux_fmt_t fmt;

    /**
     * the buffer of the yuv,
     * which is allocated in the function `pollux_decode_result_alloc`
     */
    unsigned char *buf;
} pollux_decode_result_t;

/**
 * @details
 * flow:
 *  (1) param_set   ->  pollux_decode_result_alloc
 *  (2) result_get  ->  pollux_decode_result_free
 *  (3) param_set   ->  pollux_decode_result_alloc
 *  (4) result_get  ->  pollux_decode_result_free
 *  (5) ...
 *  (6) release
 */
typedef struct pollux_decode_t {
    /* private data */
    void *priv_data;

    /**
     * @brief set parameters to the decoder;
     *  this function will request some resources, 
     *  which must be released through the `release` function
     * 
     * @param[in] thiz: the handle of type `pollux_decode_t`
     * @param[in] p_param: set parameters
     * 
     * @return 0 on success, error code otherwise
     * 
     * @note after the parameters are set,
     *  the size of the result cache `pollux_decode_result_t`
     *  may no longer be appropriate, so when you call this
     *  function, you may want to ensure that the function
     *  `result_get` exits temporarily in any thread
     */
    int (*param_set)(struct pollux_decode_t *thiz,
        const pollux_decode_param_t *p_param);

    /**
     * @brief release the resource of the decoder,
     *  this function must be used before `pollux_decode_deinit`
     * 
     * @param[in] thiz: the handle of type `pollux_decode_t`
     * 
     * @return 0 on success, error code otherwise
     */
    int (*release)(struct pollux_decode_t *thiz);

    /**
     * @brief get result
     * 
     * @param[in] thiz: the handle of type `pollux_decode_t`
     * 
     * @return 0 on success;
     * 
     *  `POLLUX_ERR_FILE_END` indicates that when the `is_loop`
     *  parameter in the `pollux_decode_param_t` struct is set to 0
     *  and the file has been decoded to the end;
     *  calling this function again at this point makes no sense
     * 
     *  `POLLUX_ERR_DECODE_THD_EXIT` indicates that the decoding
     *  thread has exited;
     *  calling this function again at this point makes no sense
     * 
     *  error code otherwise, you may be able to continue
     *  calling this function to get results
     * 
     * @note ensure that the `pollux_decode_result_t` result cache
     *  complies with `param_set` parameters
     */
    int (* result_get)(struct pollux_decode_t *thiz,
        pollux_decode_result_t *p_res);
} pollux_decode_t;

/**
 * @brief free the memory for the `pollux_decode_result_t` struct
 * 
 * @param[in] p_result: the pointer of `pollux_decode_result_t`
 */
void
pollux_decode_result_free(pollux_decode_result_t *p_result);

/**
 * @brief allocate a memory for a `pollux_decode_result_t` struct,
 *  the function `param_set` needs to be called before calling
 *  this function
 * 
 * @param[in] p_handle: the handle of type `pollux_decode_t`
 * @param[out] pp_ressult: the pointer of `pollux_decode_result_t`
 * 
 * @return 0 on success, error code otherwise
 * 
 * @note the function assigns a value to the `stride`
 *  member in the `pollux_decode_result_t` struct
 */
int
pollux_decode_result_alloc(pollux_decode_t *p_handle,
    pollux_decode_result_t **pp_ressult);

/**
 * @brief deinit the pollux decode module
 * 
 * @param[in] p_handle: the handle of type `pollux_decode_t`
 * 
 * @return 0 on success, error code otherwise
 */
int
pollux_decode_deinit(pollux_decode_t *p_handle);

/**
 * @brief init the pollux decode module
 * 
 * @param[in] p_handle: the handle of type `pollux_decode_t`
 * 
 * @return 0 on success, error code otherwise
 */
int
pollux_decode_init(pollux_decode_t **pp_handle);

#ifdef __cplusplus
}
#endif

#endif // __POLLUX_DECODE_H__
