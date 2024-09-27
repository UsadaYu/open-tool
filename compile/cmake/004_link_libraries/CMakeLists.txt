# How to link library files through cmake #
# 这里，将 003 演示中生成的静态库/动态库和头文件拷贝到当前工程根目录下；
# 并且，将 002 演示中的主函数拷贝到当前工程中，用于调用库文件中的函数

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)

project (
    PROJECT-CMAKE-LINK-LIBRARIES
    VERSION "1.0.0"
    DESCRIPTION "cmake link libraries"
    LANGUAGES "C" "CXX"
)

# 使用的库文件种类，static 为 静态库，share 为动态库
set(_lib_type share)

set(_src_dir ${PROJECT_SOURCE_DIR}/src)
set(_bin_dir ${PROJECT_BINARY_DIR}/bin)
set(_target main)

if(_lib_type STREQUAL static)
    # 静态库
    set(_lib_dir ${PROJECT_SOURCE_DIR}/lib/static)
    set(_lib_list demo)
elseif(_lib_type STREQUAL share)
    # 动态库
    set(_lib_dir ${PROJECT_SOURCE_DIR}/lib/share)
    set(_lib_list demo)
else()
    message(FATAL_ERROR "library type: ${_lib_type}")
endif()

#[[
@brief
    指定库文件的搜索路径

@usage
    link_directories(<library_path> [...])
    library_path 表示库文件的搜索路径
    [...]表示如果需要指定多个库文件搜索路径，可以继续将其它路径在后续追加

@notice
    (1) 此命令既可以指出静态库的搜索路径；也可以指出动态库的搜索路径
]]
link_directories(${_lib_dir})

include_directories(${PROJECT_SOURCE_DIR}/include/external)
aux_source_directory(${_src_dir} _src_list)

if(OFF)
    #[[
    @brief
        链接库文件的命令

    @usage
        link_libraries(<target> [...])
        target 表示链接后的目标文件名称；
        [...]表示如果需要链接多个库文件，可以继续将其它库文件名称在后续追加

    @notice
        (1) 此命令既可以链接静态库；也可以链接动态库

        (2) 此命令必须在 add_executable 或 add_library 命令前调用

        (3) 指定库名称时可以使用库文件的全名，也可以使用去除前缀和后缀的库名称；
        如，一个静态库为 libexample.a。那么使用 `link_libraries` 指定静态库链接时，
        link_libraries(libexample.a) 是对的；
        link_libraries(example) 也是对的

        (4) 如果 link_libraries 时指定完整的静态库文件路径，
        那么就不能使用去除前缀和后缀的库名称了；
        如，一个静态库为 libexample.a。那么使用 link_libraries 指定静态库链接时，
        link_libraries($HOME/cmake_demo/lib/libexample.a) 是对的；
        link_libraries($HOME/cmake_demo/lib/example) 是错的。

        (5) 此命令一般配合使用 `link_directories` 命令指出库文件的搜索目录

        (6) 开启 VERBOSE 打印，可以看到此命令在链接时添加了 -rpath 参数
    ]]
    link_libraries(${_lib_list})

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${_bin_dir})
    add_executable(${_target} ${_src_list})

else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${_bin_dir})
    add_executable(${_target} ${_src_list})

    #[[
    @brief
        链接库文件的命令

    @usage
        target_link_libraries(<target>
            <PRIVATE|PUBLIC|INTERFACE> <item>...
            [<PRIVATE|PUBLIC|INTERFACE> <item>...]...)
        (1) target 表示需要添加 item 库文件的目标文件

        (2) PRIVATE|PUBLIC|INTERFACE 表示链接权限，关于 cmake 权限，可参考：
        https://chunleili.github.io/cmake/understanding-INTERFACE

    @notice
        (1) 此命令既可以链接静态库；也可以链接动态库

        (2) 此命令必须在 `add_executable` 或 `add_library` 命令前调用

        (3) 关于链接库的名称指定，链接规则，
        类似与 `link_libraries` 命令，此处不多赘述
    ]]
    target_link_libraries(${_target} PRIVATE ${_lib_list})

endif()

if (_lib_type STREQUAL share)
    #[[
    @brief
        对目标文件添加链接选项

    @usage
        target_link_options(<target> [BEFORE]
            <PRIVATE|PUBLIC|INTERFACE> [items1...]
            [<PRIVATE|PUBLIC|INTERFACE> [items2...] ...])
        (1) target 表示需要添加 item 链接选项的目标文件；

        (2) 关于 BEFORE 参数，实际上就是将这个链接选项放到最前面的意思。
        比如：
        target_link_options(${_target} item1)
        target_link_options(${_target} item2)
        那么链接顺序是 gcc item1 item2 ...
        如果是：
        target_link_options(${_target} item1)
        target_link_options(${_target} BEFORE item2)
        那么链接顺序是 gcc item2 item1 ...

    @notice
        (1) 此命令必须在 `add_executable` 或 `add_library` 命令前调用

        (2) cmake 命令的默认权限一般都为 PUBLIC，此命令也是

        (3) 若目标文件若为静态库，此命令不起作用。
        熟悉 makefile 与 gcc 就知道，编译静态库时是不使用 ld 的，只会使用 ar，
        但是编译可执行文件与动态库时，是使用 gcc (-share) ...
        `target_link_options` 这个命令本质上类似于 gnuc 中的 ld

        (4) 链接库的选项如：-lm、-lpthread 等，可以用此选项添加，但非常不建议！
        此命令添加的选项会将选共享往前放，比如：
        gcc -lm -o demo ...
        不过实际上，链接库选项一般放在最后，并且需要循环搜索，即
        gcc -o demo -lm ...
        所以 cmake 中对于库文件的链接，
        尽量使用 `target_link_libraries` 与 `link_libraries` 等

        (5) 链接选项可能对编译器可能有一定要求，所以跨平台需要十分注意
    ]]
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
        CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_link_options(${_target} PRIVATE -Wl,-O1)
    endif()
endif()
