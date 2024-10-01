set(_src_dir ${PROJECT_SOURCE_DIR}/src)
aux_source_directory(
    ${_src_dir}
    _src_list
)
aux_source_directory(
    ${_src_dir}/internal
    _src_list_internal
)
list(APPEND _src_list ${_src_list_internal})

set(LIBRARY_OUTPUT_PATH ${TARGET_LIB_DIR})
if(BUILD_SHARED_LIBS)
    add_library(
        ${POLLUX_TARGET_NAME} SHARED ${_src_list}
    )
    target_compile_options(
        ${POLLUX_TARGET_NAME}
        PRIVATE -fPIC
    )
else()
    add_library(
        ${POLLUX_TARGET_NAME} STATIC ${_src_list}
    )
endif()

target_compile_options(
    ${POLLUX_TARGET_NAME}
    PRIVATE ${SIRIUS_CFLAGS}
)
target_compile_options(
    ${POLLUX_TARGET_NAME}
    PRIVATE ${FFMPEG_CFLAGS}
)

# `sirius` internal compile options #
target_compile_definitions(
    ${POLLUX_TARGET_NAME}
    PRIVATE -DLOG_MODULE_NAME="lib-pollux"
)
target_compile_definitions(
    ${POLLUX_TARGET_NAME}
    PRIVATE -DLOG_PRNT_BUF_SIZE=1024
)

# asan #
if(POLLUX_ASAN)
    target_compile_options(
        ${POLLUX_TARGET_NAME} PUBLIC
        -fsanitize=address
        -fno-omit-frame-pointer
        -fsanitize-recover=address
    )
    target_link_options(
        ${POLLUX_TARGET_NAME} PUBLIC
        -fsanitize=address
    )
endif()

# gcov #
if(POLLUX_GCOV)
    target_compile_options(
        ${POLLUX_TARGET_NAME} PUBLIC
        --coverage
    )
    target_link_options(
        ${POLLUX_TARGET_NAME} PUBLIC
        --coverage
    )
endif()

# all warnings #
if(POLLUX_WARNING_ALL)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(
            ${POLLUX_TARGET_NAME}
            PRIVATE /W4
        )
    else()
        target_compile_options(
            ${POLLUX_TARGET_NAME}
            PRIVATE -Wall
        )
    endif()
endif()

# compile warnings as errors #
if(POLLUX_WARNING_ERROR)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(
            ${POLLUX_TARGET_NAME}
            PRIVATE /WX
        )
    else()
        target_compile_options(
            ${POLLUX_TARGET_NAME}
            PRIVATE -Wall
        )
    endif()
endif()

# header files #
target_include_directories(
    ${POLLUX_TARGET_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include/
)
