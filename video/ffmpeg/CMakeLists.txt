cmake_minimum_required(VERSION 3.13)

project(
    PROJECT-POLLUX
    VERSION "1.0.0"
    DESCRIPTION "pollux"
    LANGUAGES "C" "CXX"
)

set(POLLUX_VERSION "1.0.0")

if(NOT DEFINED FFMPEG_PREFIX)
    message(WARNING
        "use `-DFFMPEG_PREFIX` to specify the path prefix for `ffmpeg`\n"
        "`ffmpeg` source address: https://ffmpeg.org/download.html")
    return()
endif()

if(NOT DEFINED SIRIUS_PREFIX)
    message(WARNING
        "use `-DSIRIUS_PREFIX` to specify the path prefix for `sirius`\n"
        "`sirius` source address: https://github.com/UsadaYu/sirius")
    return()
endif()

find_package(PkgConfig REQUIRED)

set(ENV{PKG_CONFIG_PATH} ${FFMPEG_PREFIX}/lib/pkgconfig)
message(STATUS
    "`PKG_CONFIG_PATH` of `ffmpeg`: $ENV{PKG_CONFIG_PATH}")
pkg_check_modules(
    FFMPEG REQUIRED
    libavformat>=61.7.100
    libavcodec>=61.19.100
    libavutil>=59.39.100
    libswresample>=5.3.100
    libswscale>=8.3.100
)

set(ENV{PKG_CONFIG_PATH} ${SIRIUS_PREFIX}/lib/pkgconfig)
message(STATUS
    "`PKG_CONFIG_PATH` of `sirius`: $ENV{PKG_CONFIG_PATH}")
pkg_check_modules(
    SIRIUS REQUIRED
    sirius>=1.0.2
)

# target information #
set(POLLUX_TARGET_NAME "pollux" CACHE STRING "target name")
set(TARGET_DIR ${PROJECT_BINARY_DIR}/artifact/)
set(TARGET_LIB_DIR ${TARGET_DIR}/lib)

add_subdirectory(cmake)

option(POLLUX_TEST_ENABLE "test enable" OFF)
if (POLLUX_TEST_ENABLE)
    enable_testing()
    add_subdirectory(unittests)
endif()
