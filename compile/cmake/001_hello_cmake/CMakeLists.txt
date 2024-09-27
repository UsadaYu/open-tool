##############################################################################
#[[
    关于 cmake 的构建与编译，一些基本用法如下：

构建
    (1) unix 默认构建
cmake -H. -Bbuild

    (2) mingw64 默认构建
cmake -G "MinGW Makefiles" -H. -Bbuild

    (3) 指定编译器为 x86 clang
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -H. -Bbuild

    (4) 指定编译器为 x86 gnuc
cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -H. -Bbuild

    (5) 指定交叉编译器，如 3519dv500
cmake \
-DCMAKE_C_COMPILER=aarch64-linux-gnu-hi3519dv500-v2-gcc \
-DCMAKE_CXX_COMPILER=aarch64-linux-gnu-hi3519dv500-v2-g++ \
-H. -Bbuild

编译
    (1) 默认，编译，等同于 --target all
cmake --build ./build

    (2) 默认编译
cmake --build ./build --target all

    (3) 编译，并打印详细编译信息
cmake --build ./build -- VERBOSE=1

    (4) 编辑 cmake cache
cmake --build ./build --target edit_cache

    (5) 清除缓存
cmake --build ./build --target clean

    (6) 安装编译后的文件到指定位置
cmake --build ./build --target install

]]
##############################################################################

# cmake 注释方式说明
    # (1). 符号 `#` 可以用于单行注释
    # (2). 符号 `#[[ ]]` 可以用于多行注释
    #[[
        hello cmake
        ]]

project(hello-cmake)
add_executable(main ./main.c)
