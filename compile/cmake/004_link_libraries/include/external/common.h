#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>

#ifdef __cplusplus
#include <iostream>
#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

#define C_INFO() \
    do { \
        printf( \
            "[File: %s] [Line: %d] [Function: %s]\n", \
            __FILE__, __LINE__, __FUNCTION__); \
    } while(0);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#define CXX_INFO() \
    do { \
        std::cout << \
        "[File: " << __FILE__ << "] " << \
        "[Line: " << __LINE__ << "] " \
        << "[Function: " << __FUNCTION__ << "]" << \
        std::endl; \
    } while(0);

#define CXX_TYPE_INFO(var) \
    do { \
        std::cout << "Type "#var": " << \
        typeid(var).name() << std::endl; \
    } while(0);
#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief C函数声明
 */
void fun_c1();
void fun_c2();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/**
 * @brief C++函数声明
 */
void fun_cxx1();
void fun_cxx2();
#endif // __cplusplus

#endif // __COMMON_H__
