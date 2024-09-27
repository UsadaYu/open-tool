######################## @Title 当前目录的Cmake脚本样例 ########################
# 此目录工程结构基本类似于 004，
# 区别在与 004 生成的是可执行文件，本工程生成的是静态库

# 生成静态库，有两种形式，举例说明：
# 假设要编译生成 B 静态库，B 静态库依赖 A 静态库，那么：
# (1) B 静态库编译时只需要 A 静态库的头文件，不需要实际的 A 静态库。
#   这样后续使用 B 静态库编译可执行文件时，需要将静态库 A 显示指定并链接；
# (2) 使用 ar 命令，将 A 静态库和 B 静态库合并在一起。
#   这样后续使用 B 静态库编译可执行文件时，不需要将静态库 A 显示指定并链接

# 关于第一种方式，即直接使用 `add_library` 命令生成一个静态库，
# 此时若在 unix 环境下，cmake 会调用 ar 命令，开启 VERBOSE 后，
# 可以看到 ar 命令只是链接了 .o 文件，
# 004 中也提到， `target_link_libraries` 与 `target_link_options`
# 命令对于 target 为静态库的情况不起作用。
# 所以外部库文件也不会被链接进去，确切来说，
# 这里所谓的链接外部库文件实际上并不是链接，而是合并，
# 在 gnuc 与 makefile 中一般采用 ar 命令实现

# 使用 `link_directories` 指定静态库 A 的头文件路径即可编译通过，这里不多赘述。
# 本工程演示如何使用 `CMAKE_AR` 命令将多个静态库相合并。
# 在 unix 中，`CMAKE_AR` 可以为 ar(gnuc)，llvm-ar 等
##############################################################################

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)

project(
	PROJECT-CMAKE-AR
	VERSION "1.0.0"
	DESCRIPTION "cmake ar"
	LANGUAGES "C" "CXX"
)

set(_src_dir ${PROJECT_SOURCE_DIR}/src)
set(_target ar_demo)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fPIE)
else()
    message(FATAL_ERROR "only gnuc supported")
endif()

# header files
include_directories(${PROJECT_SOURCE_DIR}/include/external)

# static libraries
set(_lib_dir ${PROJECT_SOURCE_DIR}/lib)
file(GLOB _static_libs "${_lib_dir}/*.a")

# source files
set(_src_dir ${PROJECT_SOURCE_DIR}/src)
aux_source_directory(${_src_dir} _src_list)

# label, create the target directory
set(_target_dir ${PROJECT_BINARY_DIR}/artifact/lib)
add_custom_command (
    OUTPUT label_target_dir_cr
    COMMAND ${CMAKE_COMMAND} -E make_directory ${_target_dir}
)

# ar
set(_target_path ${_target_dir}/lib${_target}.a)
add_library(${_target} OBJECT ${_src_list})
add_custom_command (
    OUTPUT label_ar
    COMMAND ${CMAKE_AR} crsT ${_target_path} $<TARGET_OBJECTS:${_target}> ${_static_libs}
    DEPENDS label_target_dir_cr ${_target}
)

# cmake --build ./build --target label_merge -- VERBOSE=1
add_custom_target(label_merge DEPENDS label_ar)

# install
# cmake --build ./build --target label_install
add_custom_command (
    OUTPUT label_install_copy
    COMMAND ${CMAKE_COMMAND} -E copy ${_target_path} ${PROJECT_SOURCE_DIR}/test
)
add_custom_target(label_install DEPENDS label_install_copy)
