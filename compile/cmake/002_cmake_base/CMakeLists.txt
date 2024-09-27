######################### common variables in cmake  #########################
#[[
    PROJECT_SOURCE_DIR  工程的根目录，即调用 `project()` 的 CMakeLists.txt 所在的绝对路径
    PROJECT_BINARY_DIR  运行 cmake 命令的目录，通常为 ${PROJECT_SOURCE_DIR}/build
    PROJECT_NAME        通过 `project()` 命令定义的项目名称

    CMAKE_SOURCE_DIR    顶层 CMakeLists.txt 所在的绝对路径
    CMAKE_BINARY_DIR    顶层构建目录的绝对路径

    CMAKE_CURRENT_SOURCE_DIR    当前 CMakeLists.txt 所在的绝对路径
    CMAKE_CURRENT_BINARY_DIR    当前脚本构建目录的绝对路径
    CMAKE_CURRENT_LIST_DIR      当前正在处理的 CMakeLists.txt 的绝对路径

    CMAKE_MODULE_PATH               用于指定额外的 cmake 模块(.cmake 文件)的搜索路径的列表
    CMAKE_RUNTIME_OUTPUT_DIRECTORY  可执行文件的输出目录，不指定默认在 PROJECT_BINARY_DIR 下
    CMAKE_LIBRARY_OUTPUT_DIRECTORY  库文件的输出目录，不指定默认在 PROJECT_BINARY_DIR 下
    EXECUTABLE_OUTPUT_PATH          可执行文件的输出目录，不指定默认在 PROJECT_BINARY_DIR 下
    LIBRARY_OUTPUT_PATH             库文件的输出目录，不指定默认在 PROJECT_BINARY_DIR 下
]]

#[[
    1. 无论顶层 CMakeLists.txt 路径是怎样的，都有：
    (1) PROJECT_SOURCE_DIR 和 CMAKE_SOURCE_DIR 始终是一致的；
    (2) PROJECT_BINARY_DIR 和 CMAKE_BINARY_DIR 始终是一致的。

    2. CMAKE_RUNTIME_OUTPUT_DIRECTORY 和 EXECUTABLE_OUTPUT_PATH 作用是一样的，
    EXECUTABLE_OUTPUT_PATH 多在旧版 cmake 中使用，新版建议使用 CMAKE_RUNTIME_OUTPUT_DIRECTORY
    若 CMAKE_RUNTIME_OUTPUT_DIRECTORY 和 EXECUTABLE_OUTPUT_PATH 都被指定，
    那么 CMAKE_RUNTIME_OUTPUT_DIRECTORY 优先级大于 EXECUTABLE_OUTPUT_PATH

    3. 关于库文件输出目录变量 CMAKE_LIBRARY_OUTPUT_DIRECTORY 和 LIBRARY_OUTPUT_PATH
    具体参考../003_Library
]]
##############################################################################

#[[
@brief
    指定当前 CMakeLists.txt 脚本使用的 cmake 最低版本为 3.10

@description
    新版本的 cmake 可能有一些新的命令，旧版本不支持，
    所以需要指定执行当前 CMakeLists.txt 脚本的最低版本 cmake

@notice
    此命令不是必须的，但是建议写上
]]
cmake_minimum_required(VERSION 3.10)

#[[
@brief
    设置 c++ 标准为 c++11，即 -std=c++11；同理有 set(CMAKE_CXX_STANDARD 14) 等

@description
    在执行cmake命令时也可以指定c++标准，但是建议在cmake脚本中配置CMAKE_CXX_STANDARD。
    如果希望执行cmake命令时指定c++标准，命令为：cmake .. -DCMAKE_CXX_STANDARD=11
    -D表示指定变量，此时，CMAKE_CXX_STANDARD会被写入Makefile中。
    当 CMakeLists.txt中指定了CMAKE_CXX_STANDARD，执行cmake命令时指定的CMAKE_CXX_STANDARD将不起作用

@notice
    若不指定 c++ 标准，默认使用 c++98 标准(也有说默认使用 c++11 标准)。
    我在 windows 下使用 mingw32-make-4.4.1 ，发现默认使用的是c++98标准；
    当然其它环境可能情况不同，所以最好将 CMAKE_CXX_STANDARD 变量显示地配置。
]]
set(CMAKE_CXX_STANDARD 11)

#[[
@brief
    配置工程信息

@notice
    一般整个工程可以有多个 CMakeLists.txt，但只有顶层的 CMakeLists.txt 能调用 project
    ]]
project (
    PROJECT-CMAKE-BASE          # 构建项目时指定的工程名称。可选参数，但建议写上
    VERSION "1.0.0"             # 项目的版本号。可选参数
    DESCRIPTION "cmake base"    # 项目的描述信息。可选参数
    LANGUAGES "C" "CXX"         # 项目语言，主流语言 C、CXX、OBJC 等 cmake 一般都支持。可选参数

    # 工程主页的 URL。可选参数
    HOMEPAGE_URL "https://github.com/UsadaYu/open-tool/tree/master"
)
message(STATUS "project name: ${PROJECT_NAME}")    # 打印信息：PROJECT-CMAKE-BASE

#[[
@brief
    设置变量，多个变量之间以空格或符号 ; 分隔。变量默认是 string 类型

@usage
    set(<VARIABLE> <variable_contents>)
    将 variable_contents 用 VARIABLE 变量代替

@notice
    使用变量时以 ${} 的方式引用；类似于 Makefile 的 $()
    一般文件内私有变量可以在前面加上 `_` 符号
]]
set(_src_dir ${PROJECT_SOURCE_DIR}/src)
set(_target main)

#[[
@brief
    设置可执行文件的输出目录

@notice
    如果目录不存在，cmake 会自动创建这个目录。即构建时就创建这个输出目录。
    若目录层级是多层的，那么将会递归地创建目录，即相当于：mkdir -p dir_path
]]
# 创建可执行文件的输出目录；因为指定 CMAKE_RUNTIME_OUTPUT_DIRECTORY 后会自动创建，所以可不写
# file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
message(STATUS "artifact directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

#[[
@brief
    指定头文件路径

@notice
    非递归
]]
include_directories(${PROJECT_SOURCE_DIR}/include)

#[[
@brief
    添加源文件到变量方式1，使用 file 命令

@usage
    file(GLOB/GLOB_RECURSE <VARIABLE> <dir_path/*.file_format>)
    GLOB 表示搜索符合 dir_path/*.file_format 的文件；
    GLOB_RECURSE 表示递归地搜索符合 dir_path/*.file_format 的文件。
    搜索后将符合指定类型的文件添加到 VARIABLE 变量中

@notice
    (1) GLOB 参数下，源文件的搜索深度固定是1，没有递归搜索，
    类似于 shell 的：find <dir_path> -maxdepth 1 -name "*.c"

    (2) GLOB_RECURSE 参数下，源文件的搜索是递归的，无视目录深度，
    类似于 shell 的：find <dir_path> -name "*.c"
]]
# Method 1 for adding source files: file(GLOB ...)
file(GLOB _src_list "${_src_dir}/*.cpp" "${_src_dir}/C/*.c" "${_src_dir}/CXX/*.cpp")
# Method 2 for adding source files: file(GLOB_RECURSE ...)
# file(GLOB_RECURSE _src_list "${_src_dir}/*.cpp" "${_src_dir}/*.c")

#[[
@brief
    添加源文件到变量方式2，使用 aux_source_directory 命令

@usage
    aux_source_directory(<dir_path> <VARIABLE>)
    搜索 dir_path 目录下的源文件(.c/.cpp/.cc等)，将其添加到 VARIABLE 变量中

@notice
    源文件的搜索深度固定是1，没有递归搜索，
    类似于 shell 的：find <dir_path> -maxdepth 1 -name "*.c"
]]
# Method 3 for adding source files: aux_source_directory(...)
# aux_source_directory(${_src_dir} _src_list)
# aux_source_diresrc_file ctory(${_src_dir}/C _src_list)
# aux_source_directory(${_src_dir}/CXX _src_list)

foreach(src_file ${_src_list})
    message(STATUS "source file: ${src_file}")
endforeach()

#[[
@brief
    基于源文件 ${_src_list}，生成名为 ${_target} 的可执行文件

@usage
    add_executable(<executable_name> <_src_list>)
    executable_name 表示生成的可执行文件的文件名；
    _src_list 表示所有需要的源文件
]]
add_executable(${_target} ${_src_list})
