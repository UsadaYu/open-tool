# test #

add_subdirectory(cmake)

set(_artifact_path "${CMAKE_CURRENT_BINARY_DIR}/artifact/")
set(_artifact_inc_path "${_artifact_path}/include/pollux")
set(_artifact_bin_path "${_artifact_path}/bin/")

file(GLOB _header_list "${PROJECT_SOURCE_DIR}/include/*.h")
foreach(file ${_header_list})
    file(COPY ${file} DESTINATION ${_artifact_inc_path})
endforeach()

file(GLOB _test_video "${CMAKE_CURRENT_SOURCE_DIR}/input*.*")
foreach(file ${_test_video})
    file(COPY ${file} DESTINATION ${_artifact_bin_path})
endforeach()

set(_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB _src_list "${_src_dir}/*.cpp" "${_src_dir}/*.c")

foreach(file ${_src_list})
    get_filename_component(file_name ${file} NAME_WE)
    set(_target_name "${POLLUX_TARGET_NAME}_${file_name}")

    add_executable(${_target_name} ${file})
    set_target_properties(
        ${_target_name}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${_artifact_bin_path}
    )

    target_include_directories(
        ${_target_name}
        PRIVATE ${_artifact_inc_path}
    )

    foreach(path ${POLLUX_TEST_EXTRA_LINK_DIR})
        target_link_directories(${_target_name} PRIVATE ${path})
    endforeach()
    target_link_directories(${_target_name} PRIVATE ${TARGET_LIB_DIR})
    target_link_directories(${_target_name} PRIVATE ${SIRIUS_LIBRARY_DIRS})
    target_link_directories(${_target_name} PRIVATE ${FFMPEG_LIBRARY_DIRS})

    target_link_libraries(${_target_name} ${POLLUX_TEST_EXTRA_LINK_LIBRARIES})
    target_link_libraries(${_target_name} ${POLLUX_TARGET_NAME})
    target_link_libraries(${_target_name} ${SIRIUS_LIBRARIES})
    target_link_libraries(${_target_name} ${FFMPEG_LIBRARIES})

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        #[[
            ffmpeg's pkgconfig file uses `-pthread` instead of
            `-lpthread` for the link option to the thread library,
            therefore, cmake's `pkg_check_modules` command cannot
            recognize it.
            so the option to link the thread library is added
            here manually

            ffmpeg version: 7.1
            cmake version: 3.30.2
            date: 2024-10-12
        ]]
        target_link_libraries(${_target_name} pthread)
    endif()

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR
        ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        if (${POLLUX_TEST_PIE_ENABLE})
            target_compile_options(${_target_name} PRIVATE -fPIE)
            target_link_options(${_target_name} PRIVATE -pie)
        else()
            target_link_options(${_target_name} PRIVATE -no-pie)
        endif()
    endif()

    add_test(
        NAME ${_target_name}
        COMMAND ./${_target_name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/artifact/bin/
    )
endforeach()
