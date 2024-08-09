/********************************************************************************************
 * @brief   此用例中，进程不会结束，开启编写内存泄露选项后，部分内存未被释放。
 *          因为进程不会退出，故MALLOC_CONF选项中的prof_final参数不起作用
 * 
 * @details
 * (1)  make clean
 *      make plat=x86 process_type=not_exit -B
 */
        // mkdir -p -m 777 ./heap && rm -rf ./heap/*
 /*
 * (2)  不退出的进程配置的环境变量与退出的进程稍有不同，一般如下：
 *      export MALLOC_CONF=\
 *      prof:true,prof_leak:true,lg_prof_interval:18,lg_prof_sample:16,prof_prefix:./heap/x86
 * 
 * (3)  上述环境变量中：
 *      prof:true           开启后，剖析进程的内存申请操作
 *      prof_leak:true      开启后，生成转储后的内存报告文件
 *      lg_prof_interval:18 生成转储文件的间隔，2^18字节
 *      lg_prof_sample:16   内存分配采样的间隔，2^16字节
 *      prof_prefix:./heap/x86  转储的heap文件前缀
 * 
 * (4)  进程运行一段时间后，选取前后几份heap文件，通过jeprof解析。
 * 
 * (5)  txt文件：
 *      1) 第10份heap文件，解析后：
 *      Total: 196035 B
 *             0   0.0%   0.0%   115363  58.8% _GLOBAL__sub_I_eh_alloc.cc
 *        115363  58.8%  58.8%   115363  58.8% _GLOBAL__sub_I_eh_alloc.cc (inline)
 *             0   0.0%  58.8%   115363  58.8% __do_global_ctors_aux
 *             0   0.0%  58.8%   115363  58.8% __static_initialization_and_destruction_0 (inline)
 *             0   0.0%  58.8%    80672  41.2% __libc_start_main
 *             0   0.0%  58.8%    80672  41.2% _start
 *         80672  41.2% 100.0%    80672  41.2% ex_alloc
 *             0   0.0% 100.0%    80672  41.2% main
 *      2) 第221份heap文件，解析后：
 *      Total: 599397 B
 *             0   0.0%   0.0%   484034  80.8% __libc_start_main
 *             0   0.0%   0.0%   484034  80.8% _start
 *        484034  80.8%  80.8%   484034  80.8% ex_alloc
 *             0   0.0%  80.8%   484034  80.8% main
 *             0   0.0%  80.8%   115363  19.2% _GLOBAL__sub_I_eh_alloc.cc
 *        115363  19.2% 100.0%   115363  19.2% _GLOBAL__sub_I_eh_alloc.cc (inline)
 *             0   0.0% 100.0%   115363  19.2% __do_global_ctors_aux
 *             0   0.0% 100.0%   115363  19.2% __static_initialization_and_destruction_0 (inline)
 *      可以看到main函数的内存占用显著增加，并且进一步指向了ex_alloc。说明ex_alloc是导致内存泄漏的元凶。
 * 
 * (6)  根据heap文件作对比。
 *      jeprof脚本提供了heap文件比对的功能。运行命令：
 *      jeprof ./bin/x86/main --base=./heap/x86.1301311.10.i10.heap ./heap/x86.1301311.221.i221.heap --text
 *      打印如下：
 *      Using local file ./bin/x86/main.
 *      Using local file ./heap/x86.1301311.221.i221.heap.
 *      Total: 0.4 MB
 *           0.4 100.0% 100.0%      0.4 100.0% ex_alloc
 *           0.0   0.0% 100.0%      0.4 100.0% __libc_start_main
 *           0.0   0.0% 100.0%      0.4 100.0% _start
 *           0.0   0.0% 100.0%      0.4 100.0% main
 *      上述，有正符号说明内存存在泄漏，可以看到是ex_alloc函数导致的，从10~221泄漏了约0.4MB。
 * 
 * (7)  dot文件：
 *      文本的形象程度不如图片，jeprof提供了根据heap文件生成dot文件的功能。
 *      进一步的，通过一些工具可将dot文件转换为html等格式的文件。
 *      jeprof --show_bytes ./bin/x86/main ./heap/x86.1301311.10.i10.heap --dot > ./x86.1301311.10.i10.dot
 *      jeprof --show_bytes ./bin/x86/main ./heap/x86.1301311.221.i221.heap --dot > x86.1301311.221.i221.dot
 *      生成dot文件后，我这里使用graphviz将dot转为html：
 *      dot -Tsvg ./x86.1301311.10.i10.dot > ./x86.1301311.10.i10.html
 *      dot -Tsvg ./x86.1301311.221.i221.dot > ./x86.1301311.221.i221.html
 *      上述一系列文件，存放在material目录下，供参考
********************************************************************************************/

#include <pthread.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include "common.h"

#define EX_HEAP_SIZE        (6666)
#define EX_HEAP_NUMBER      (5)
// CFLAGS += -DEX_LEAK_FLAG
#ifdef EX_LEAK_FLAG
#define EX_IS_LEAK          (0)     // 堆区内存完全释放标志。0：完全释放
#else
#define EX_IS_LEAK          (1)     // 堆区内存完全释放标志。1：不完全释放
#endif  // EX_LEAK_FLAG

/********************************************************************************************
 * @details
 * (1)  计算得str_arr_t结构体大小为：
 *      sizeof(int) * EX_HEAP_SIZE = 26664(B)
 *      每次泄露的结构体内存数量为1，根据采样间隔公式有：
 *      2 ^ x = 26664 * 1
 *      进一步有：
 *      x = log2 ^ 26664 ≈ 14.703
 *      所以此处采样间隔可设置为15或16
 * 
 * (2)  实际定位内存问题时，假设存在内存泄漏，可以通过top等命令观察泄漏的严重程度。
 *      假设泄漏的比较大，那么lg_prof_interval和lg_prof_sample可以配置的大一些，
 *      只要内存时持续增长的，那么采样间隔即使比较大，只要运行一段时间，一般也可以定位到问题
********************************************************************************************/
typedef struct {
    int elem[EX_HEAP_SIZE];
} str_arr_t;

void ex_heap_thrd(bool *thread_state)
{
    bool        *thread_flag    = thread_state;
    str_arr_t   *ptr            = NULL;

    while (*thread_flag) {
        do {
            printf("%lu, %s\n", syscall(__NR_gettid), __FUNCTION__);
        } while(0);

        ptr = ex_alloc(1 * sizeof(str_arr_t));
        if (!(ptr)) {
            exit(-1);
        }
        usleep(1500 * 1000);

        free(ptr);
        ptr = NULL;
    }
}

int main()
{
    int         i                           = 3600;
    int         j                           = 0;
    str_arr_t   *ptr_rise[EX_HEAP_NUMBER]   = {NULL};
    str_arr_t   *ptr_steady[EX_HEAP_NUMBER] = {NULL};

    pthread_t tid;
    bool thread_state = true;
    (void)pthread_create(&tid, NULL, (void*)ex_heap_thrd, &thread_state);

    while (i--) {
        do {
            printf("%lu, %s\n", syscall(__NR_gettid), __FUNCTION__);
        } while(0);

        for (j = 0; j < EX_HEAP_NUMBER; j++) {
            ptr_rise[j] = ex_alloc(1 * sizeof(str_arr_t));
            if (!(ptr_rise[j])) {
                 exit(-1);
            }
            usleep(200 * 1000);
        }

        for (j = 0; j < EX_HEAP_NUMBER; j++) {
            ptr_steady[j] = ex_alloc(1 * sizeof(str_arr_t));
            if (!(ptr_steady[j])) {
                 exit(-1);
            }
            usleep(150 * 1000);
        }

        // ptr_rise: EX_IS_LEAK为1时少释放一块内存
        for (j = 0; j < (EX_HEAP_NUMBER - EX_IS_LEAK); j++) {
            if (ptr_rise[j]) {
                free(ptr_rise[j]);
                ptr_rise[j] = NULL;
            }
        }

        // ptr_steady: 正常释放
        for (j = 0; j < EX_HEAP_NUMBER; j++) {
            if (ptr_steady[j]) {
                free(ptr_steady[j]);
                ptr_steady[j] = NULL;
            }
        }
    }

    thread_state = false;
    (void)pthread_join(tid, NULL);

    printf("%s accomplish\n", __FUNCTION__);
    return 0;
}
