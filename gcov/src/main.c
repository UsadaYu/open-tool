/*
要生成覆盖率html格式报告，可以运行如下命令：
make clean
make -B
./bin/x86/main
gcovr -r ./ \
--gcov-ignore-parse-errors \
--exclude-unreachable-branches \
--print-summary \
--html-details=./coverage_report/x86_coverage_repoet.html
*/

/********************************************************************************************
 * @brief   gcovr简单的使用方法
 * 
 * @details
 * (1)  编译并运行程序：
        make clean
        make -B
        ./bin/x86/main
 * 
 * (2)  运行如下命令生成覆盖率报告：
        gcovr -r ./ \
        --gcov-ignore-parse-errors \
        --exclude-unreachable-branches \
        --print-summary \
        --html-details=./coverage_report/x86_coverage_repoet.html
 * 
 * @ref     https://gcovr.com/en/stable/
********************************************************************************************/

#include <stdio.h>

#define LOOP_NUM        (8)
#define INFO_PRINT(var) \
    do { \
        printf("Line: %d, "#var" = %d\n", __LINE__, var); \
    } while(0);

// 调用此函数
static inline void func_valid()
{
    int var = 10;
    INFO_PRINT(var);
}

// 不调用此函数
static inline void func_invalid()
{
    int var = 10;
    INFO_PRINT(var);
}

int main()
{
    for (unsigned int i = 0; i < LOOP_NUM; i++) {
        switch (i) {
            case 1:
                INFO_PRINT(i);
                break;
            case 4:
                INFO_PRINT(i);
                break;
            case 10:
                INFO_PRINT(i);
                break;  
            default:
                break;
        }

        if (i == LOOP_NUM + 1) {
            INFO_PRINT(i);
        } else {
            INFO_PRINT(i);
        }

        // 此类if模块不会被覆盖率检测，覆盖率报告对此类模块呈现为白色
        if (0 == 1) {
            INFO_PRINT(i);
        }
    }

    func_valid();

    return 0;
}
