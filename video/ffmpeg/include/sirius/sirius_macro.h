/**
 * @name sirius_macro.h
 * 
 * @author 胡益华
 * 
 * @date 2024-07-30
 * 
 * @brief common macro definitions
 */

#ifndef __SIRIUS_MACRO_H__
#define __SIRIUS_MACRO_H__

#ifndef container_of
#ifndef offsetof
/**
 * @brief get the address offset of the struct member
 * 
 * @param TYPE: struct type
 * @param MEMBER: struct member
 */
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif // offsetof

/**
 * @brief get the struct head address from the struct member
 * 
 * @param ptr: the address of the struct member
 * @param type: struct type
 * @param member: struct member name
 */
#define container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type, member) );})
#endif // container_of

#ifndef SIRIUS_POINTER_ALIGN8
#if defined(__GNUC__) || defined (__clang__)
/**
 * @brief struct pointer variable aligned to 8 bytes
 * 
 * @param N: the name of the pointer variable
 * 
 * @note only GNUC compilation is supported
 */
#define SIRIUS_POINTER_ALIGN8(N)    unsigned char unused##N[8 - sizeof(void*)];

#else
#define SIRIUS_POINTER_ALIGN8(N)
#endif
#endif // SIRIUS_POINTER_ALIGN8

#if defined(__GNUC__) || defined (__clang__)
#ifndef likely
/**
 * @brief the probability of selecting a branch is high
 */
#define likely(x)       __builtin_expect(!!(x), 1)
#endif // likely

#ifndef unlikely
/**
 * @brief the probability of selecting a branch is low
 */
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif // unlikely

#else
#ifndef likely
#define likely(x)       !!(x)
#endif  // likely

#ifndef unlikely
#define unlikely(x)     !!(x)
#endif // unlikely
#endif

#if defined (__GNUC__) || defined (__clang__)
#ifndef always_inline
/**
 * @brief force inline
 */
#define always_inline \
inline __attribute__((always_inline))
#else
#define always_inline inline
#endif
#endif

#endif // __SIRIUS_MACRO_H__
