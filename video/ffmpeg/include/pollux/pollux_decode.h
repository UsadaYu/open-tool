#ifndef __POLLUX_DECODE_H__
#define __POLLUX_DECODE_H__

typedef enum {
    POLLUX_DECODE_FMT_NONE = -1,

    /* yuv nv21 */
    POLLUX_DECODE_FMT_NV21,

    POLLUX_DECODE_FMT_MAX,
} pollux_decode_fmt_t;

typedef struct {
    /* width */
    unsigned short width;
    /* height */
    unsigned short height;

    /* the buffer size alignment for the yuv */
    int alignment;

    /* format, refer to `pollux_decode_fmt_t` */
    pollux_decode_fmt_t fmt;
} pollux_decode_yuv_t;

typedef struct {
    /* frames per second */
    unsigned short fps;

    /** 
     * cyclic or not decoding,
     * 1: cyclic decoding; 0: non-cyclic decoding
     */
    unsigned short is_loop;

    /*
     * information of the yuv settings,
     * make sure that it is equal to the parameter in the
     * function `pollux_decode_result_alloc`
     */
    pollux_decode_yuv_t yuv;

    /* source stream file path */
    char *p_file;
} pollux_decode_param_t;

typedef struct {
    /* width */
    unsigned short width;
    /* height */
    unsigned short height;
    /* stride */
    unsigned short stride;

    /* format, refer to `pollux_decode_fmt_t` */
    pollux_decode_fmt_t fmt;

    /**
     * the buffer of the yuv,
     * which is allocated in the function `pollux_decode_result_alloc`
     */
    unsigned char *buf;
} pollux_decode_result_t;

/**
 * @brief free the memory for the
 * `pollux_decode_result_t` struct, and set it to null
 * 
 * @param[in] p_result: the pointer of `pollux_decode_result_t`
 */
void
pollux_decode_result_free(pollux_decode_result_t *p_result);

/**
 * @brief allocate a memory for a
 * `pollux_decode_result_t` struct
 * 
 * @param[in] p_yuv: information of the yuv result cache,
 *  make sure that it is equal to the parameter in the
 *  function `param_set`
 * 
 * @return failure if return is null, otherwise success
 * 
 * @note the function assigns a value to the
 *  `stride` member in the return struct
 */
pollux_decode_result_t *
pollux_decode_result_alloc(pollux_decode_yuv_t *p_yuv);

/**
 * @details
 * flow:
 *  (1) param_set (pollux_decode_result_alloc)
 *  (2) result_get
 *  (3) param_set (pollux_decode_result_free - pollux_decode_result_alloc)
 *  (4) result_get
 *  (5) ...
 *  (6) release
 */
typedef struct pollux_decode_t {
    /* private data */
    void *priv_data;

    /**
     * @brief set parameters to the decoder,
     *  make sure the decoder is uninitialized before using this;
     *  this function will request some resources, 
     *  which must be released through the `release` function
     * 
     * @param[in] thiz: the handle of type `pollux_decode_t`
     * @param[in] p_param: the set parameter
     * 
     * @return 0 on success, error code otherwise
     * 
     * @note after the parameters are set,
     *  the size of the result cache `pollux_decode_result_t`
     *  may no longer be appropriate, so when you call this function,
     *  you may want to ensure that the function `result_get`
     *  exits temporarily in any thread
     */
    int (*param_set)(struct pollux_decode_t *thiz,
        const pollux_decode_param_t *p_param);

    /**
     * @brief release the resource of the decoder,
     *  the function must be used before `pollux_decode_deinit`
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
     *  `POLLUX_ERR_NOT_INIT` indicates that the parameter
     *  is being set, just need to call the function again;
     * 
     *  `POLLUX_ERR_DECODE_THD_EXIT` indicates that the 
     *  decoding thread has stopped and the function `release`
     *  needs to be called immediately to release the appropriate
     *  resource;
     * 
     *  error code otherwise, you may be able to continue
     *  calling this function to get results
     * 
     * @note ensure that the `pollux_decode_result_t`
     *  result cache complies with `param_set` parameters
     */
    int (* result_get)(struct pollux_decode_t *thiz,
        pollux_decode_result_t *p_res);
} pollux_decode_t;

/**
 * @brief deinit the pollux decode module
 * 
 * @param[in] p_handle: the handle of the `pollux_decode`
 * 
 * @return 0 on success, error code otherwise
 */
int
pollux_decode_deinit(pollux_decode_t *p_handle);

/**
 * @brief init the pollux decode module
 * 
 * @param[in] p_handle: the handle of the `pollux_decode`
 * 
 * @return 0 on success, error code otherwise
 */
int
pollux_decode_init(pollux_decode_t **pp_handle);

#endif // __POLLUX_DECODE_H__
