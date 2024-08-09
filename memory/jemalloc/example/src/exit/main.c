/********************************************************************************************
 * @brief   此用例中，进程运行完毕结束，这种情况下使用jemalloc定位内存问题会相对容易
 * 
 * @details
 * (1)  make clean
 *      make plat=x86 process_type=exit -B
 */
        // mkdir -p -m 777 ./heap && rm -rf ./heap/*
 /*
 * 
 * (2)  要使用jemalloc统计内存，需要配置环境变量，这里有两种配置方式。
 *          方法1：在外部配置环境变量，即：
 *      export MALLOC_CONF=\
 *      prof:true,prof_leak:true,lg_prof_sample:0,prof_final:true,prof_prefix:./heap/x86
 *          方法2：在代码内配置环境变量，在代码中定义名为malloc_conf的全局字符串，
 *          jemalloc会读取这个字符串作为环境变量。
 *          const char *malloc_conf = \
 *      "prof:true,prof_leak:true,lg_prof_sample:0,prof_final:true,prof_prefix:./heap/x86"
 * 
 * (3)  上述环境变量中：
 *      prof:true           开启后，剖析进程的内存申请操作
 *      prof_leak:true      开启后，生成转储后的内存报告文件
 *      lg_prof_sample:0    内存分配采样的间隔，2^0字节
 *      prof_final:true     基于atexit函数转储进程结束时的内存情况
 *      prof_prefix:./heap/x86  转储的heap文件前缀
 * 
 * (4)  进程结束后，使用jemalloc提供的jeprof脚本分析heap文件，生成的结果选择很多，如txt、dot等，
 *      可自行选择，这里举个生成txt文本文件的例子。
 *      jeprof --show_bytes ./bin/x86/main ./heap/x86.*.0.f.heap --text --cum
 * 
 * (5)  简单的泄露通过文本文件基本就能看出。
 *      1) make plat=x86 process_type=exit is_leak=false -B，堆区内存已释放文本结果：
 *      Using local file ./bin/x86/main.
 *      Using local file ./heap/x86.1301036.0.f.heap.
 *      Total: 81920 B
 *             0   0.0%   0.0%    81920 100.0% _GLOBAL__sub_I_eh_alloc.cc
 *         81920 100.0% 100.0%    81920 100.0% _GLOBAL__sub_I_eh_alloc.cc (inline)
 *             0   0.0% 100.0%    81920 100.0% __do_global_ctors_aux
 *             0   0.0% 100.0%    81920 100.0% __static_initialization_and_destruction_0 (inline)
 *      这意味着内存是被完全释放的
 *      2) 堆区内存未释放文本结果：
 *      Using local file ./bin/x86/main.
 *      Using local file ./heap/x86.1300932.0.f.heap.
 *      Total: 98304 B
 *             0   0.0%   0.0%    81920  83.3% _GLOBAL__sub_I_eh_alloc.cc
 *         81920  83.3%  83.3%    81920  83.3% _GLOBAL__sub_I_eh_alloc.cc (inline)
 *             0   0.0%  83.3%    81920  83.3% __do_global_ctors_aux
 *             0   0.0%  83.3%    81920  83.3% __static_initialization_and_destruction_0 (inline)
 *             0   0.0%  83.3%    16384  16.7% __libc_start_main
 *             0   0.0%  83.3%    16384  16.7% _start
 *         16384  16.7% 100.0%    16384  16.7% ex_alloc
 *             0   0.0% 100.0%    16384  16.7% main
 *      可以看到，main函数有残留的内存没有被释放，进一步可以定位到ex_alloc函数中；
 *      说明ex_alloc函数中申请的内存没有被释放
********************************************************************************************/

#include "common.h"

#define EX_HEAP_NUMBER      (4096)

// CFLAGS += -DEX_LEAK_FLAG
#ifdef EX_LEAK_FLAG
#define EX_IS_LEAK          (0)     // 堆区内存完全释放标志。0：不释放
#else
#define EX_IS_LEAK          (1)     // 堆区内存完全释放标志。1：释放
#endif  // EX_LEAK_FLAG

int main()
{
    int i       = 0;

    int *ptr = (int *)ex_alloc(sizeof(int) * EX_HEAP_NUMBER);
    if (!(ptr)) {
        exit(-1);
    }

    for (i = 0; i < EX_HEAP_NUMBER; i++) {
        ptr[i] = i;
    }

#if (EX_IS_LEAK)
    free(ptr);
    ptr = NULL;
    printf("%s, release\n", __FUNCTION__);
#else
    printf("%s, leak\n", __FUNCTION__);
#endif

    return 0;
}
