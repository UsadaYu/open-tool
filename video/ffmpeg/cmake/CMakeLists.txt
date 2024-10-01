# compiler information #
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
        message(WARNING
            "current gnuc version: ${CMAKE_CXX_COMPILER_VERSION}\n"
            "gnuc version must be at least 4.9\n"
            "please edit cmake cache or rebuild")
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.1")
        message(WARNING
            "current clang version: ${CMAKE_CXX_COMPILER_VERSION}\n"
            "clang version must be at least 3.1\n"
            "please edit cmake cache or rebuild")
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16.8")
        message(WARNING
            "current msvc version: ${CMAKE_CXX_COMPILER_VERSION}\n"
            "msvc version must be at least 16.8\n"
            "please edit cmake cache or rebuild")
    endif()
else()
    message(FATAL_ERROR
        "${CMAKE_CXX_COMPILER_ID} is not supported")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/external_option.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/internal_option.cmake)

install(
    TARGETS ${POLLUX_TARGET_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/pollux.pc.in
    ${TARGET_LIB_DIR}/pollux.pc
    @ONLY
)
install(
    FILES ${TARGET_LIB_DIR}/pollux.pc
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
    FILES_MATCHING PATTERN "*.h"
    PATTERN "internal" EXCLUDE
)
