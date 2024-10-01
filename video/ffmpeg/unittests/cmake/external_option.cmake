# instruction #

option(
    POLLUX_TEST_PIE_ENABLE
    "position independent, enable `pie`" OFF
)

set(
    POLLUX_TEST_EXTRA_LINK_DIR 
    "" CACHE STRING
    "the link library file path in the test, e.g., `/path/lib1;/path/lib2`"
)

set(
    POLLUX_TEST_EXTRA_LINK_LIBRARIES 
    "" CACHE STRING
    "the link library file name in the test, e.g., `pthread;stdc++`"
)
