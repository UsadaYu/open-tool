# how to compile library files with cmake #

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)

project (
    PROJECT-ADD-LIBRARY
    VERSION "1.0.0"
    DESCRIPTION "cmake libraries make"
    LANGUAGES "C" "CXX"
)

#[[
@brief
	build library type: static or shared

	edit it with this command:
	cmake --build ./build/ --target edit_cache
]]
option(BUILD_SHARED_LIBS "build shared libraries" OFF)

set(_src_dir ${PROJECT_SOURCE_DIR}/src)
set(_artifact_dir ${PROJECT_BINARY_DIR}/artifact)
set(_tatget demo)

#[[
@brief
    指定库文件生成路径

@notice
    (1) 动态库有可执行权限，若需要指定动态库生成路径，可以用以下参数：
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
    EXECUTABLE_OUTPUT_PATH
    CMAKE_LIBRARY_OUTPUT_DIRECTORY
    LIBRARY_OUTPUT_PATH

    (2) 静态库没有可执行权限，若需要指定静态库生成路径，可以用以下参数：
    LIBRARY_OUTPUT_PATH

    (3) 综上所述，要生成库文件，无论是静态库或动态库，
	选择 LIBRARY_OUTPUT_PATH 参数比较好
]]
set(LIBRARY_OUTPUT_PATH ${_artifact_dir})

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE _src_list "${_src_dir}/*.cpp" "${_src_dir}/*.c")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(BUILD_SHARED_LIBS)
        add_compile_options(-fPIC)
    else()
        add_compile_options(-fPIE)
    endif()
endif()

#[[
@brief
    生成静态库或动态库命令

@usage
    add_library(<library_name> STATIC/SHARED <_src_list>)
    library_name 表示生成的库文件的文件名；
    `STATIC` 表示生成静态库，`SHARED` 表示生成动态库；
    _src_list 表示所有需要的源文件

@notice
    (1) cmake 是跨平台的，所以不需要指出库文件的文件后缀，cmake 会自行处理。
    一般 Linux 的静态库后缀为 .a，动态库后缀为 .so；
    一般 Windows 的静态库后缀为 .lib，动态库后缀为 .dll

    (2) Windows 下使用 mingw64 生成动态库时，还可能伴随着一个 *.dll.a 的文件生成，
    这一般是在使用 GNU 工具链的情况下会看到，这个文件是为了与 GNU 工具链兼容而生成的

    (3) Linux/Windows 等系统下生成的库文件固定前缀都为lib。
    library_name 中库文件前缀和后缀都不需要显示指出，只需要指定文件名即可
]]
if(BUILD_SHARED_LIBS)
    add_library(${_tatget} SHARED ${_src_list})
else()
    add_library(${_tatget} STATIC ${_src_list})
endif()

# 将头文件添加到目标目录下，也可用 cmake 提供的 `install` 等命令进行安装
file(COPY ${PROJECT_SOURCE_DIR}/include DESTINATION ${_artifact_dir})
