#ifndef __POLLUX_INTERNAL__
#define __POLLUX_INTERNAL__

typedef enum {
    /* invalid state */
    INTERNAL_THD_STATE_INVALID = 0,

    /* thread exiting */
    INTERNAL_THD_STATE_EXITING,
    /* thread exited */
    INTERNAL_THD_STATE_EXITED,
    /* thread is running */
    INTERNAL_THD_STATE_RUNNING,

    /* thread self-termination */
    INTERNAL_THD_STATE_TERMINATION,

    INTERNAL_THD_STATE_MAX,
} pollux_internal_thd_state_t;

#endif // __POLLUX_INTERNAL__
