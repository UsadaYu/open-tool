#ifndef __POLLUX_ERRNO_H__
#define __POLLUX_ERRNO_H__

#define POLLUX_OK                       (0)             // 通用正确
#define POLLUX_ERR                      (-1)            // 通用错误

/* common */
#define POLLUX_ERR_TIMEOUT              (-10000)        // 超时
#define POLLUX_ERR_NULL_POINTER         (-10001)        // 指针为空
#define POLLUX_ERR_INVALID_PARAMETER    (-10002)        // 参数无效

/* function */
#define POLLUX_ERR_INVALID_ENTRY        (-11000)        // 函数入参无效
#define POLLUX_ERR_INIT_REPEATED        (-11001)        // 重复初始化
#define POLLUX_ERR_NOT_INIT             (-11002)        // 未初始化

/* resource */
#define POLLUX_ERR_MEMORY_ALLOC         (-12000)        // 内存申请失败
#define POLLUX_ERR_CACHE_OVERFLOW       (-12001)        // 缓存不足
#define POLLUX_ERR_RESOURCE_REQUEST     (-12002)        // 资源申请失败

/* decode */
#define POLLUX_ERR_DECODE_THD_EXIT      (-15000)        // 解码线程已退出

#endif // __POLLUX_ERRNO_H__
