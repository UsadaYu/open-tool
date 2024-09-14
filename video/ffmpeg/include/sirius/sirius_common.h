/**
 * @name sirius_common.h
 * 
 * @author 胡益华
 * 
 * @date 2024-07-30
 */

#ifndef __SIRIUS_COMMON_H__
#define __SIRIUS_COMMON_H__

#include "sirius_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SIRIUS_VERSION_MAJOR 1
#define SIRIUS_VERSION_MINOR 0
#define SIRIUS_VERSION_PATCH 1

#define SIRIUS_VERSION "1.0.1"

/**
 * @brief get version
 */
char *
sirius_get_version();

typedef struct {
    /* default log level */
    sirius_log_lv_t log_lv;
    /* pipe path */
    char *p_pipe;
} sirius_init_t;

/**
 * @brief sirius deinit
 */
void
sirius_deinit();

/**
 * @brief sirius init, use `sirius_deinit` to deinitialize
 * 
 * @param[in]  p_init: init parameter
 * 
 * @note the thread is unsafe
 * 
 * @return 0 on success, error code otherwise
 */
int
sirius_init(sirius_init_t *p_init);

#ifdef __cplusplus
}
#endif

#endif  // __SIRIUS_COMMON_H__
