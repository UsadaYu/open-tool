#/bin/bash

_internal_info_base="[$(date "+%Y-%m-%d %H:%M:%S") $0]"

info_red() {
    echo -e "\033[0;31m$1\033[0m"
}

info_green() {
    echo -e "\033[0;32m$1\033[0m"
}

info_yellow() {
    echo -e "\033[0;33m$1\033[0m"
}

info_blue() {
    echo -e "\033[0;34m$1\033[0m"
}

info_base() {
    info_green "${_internal_info_base}"
}

error_print() {
    if [ -n "$1" ]; then local error_code=$1
    else error_code=-1
    fi

    info_red "${_internal_info_base}"
    info_red "An error occurred when the script was triggered"
    info_red "Error code: $error_code"
}

error_exit_print() {
    if [ -n "$1" ]; then local error_code=$1
    else error_code=-1
    fi

    info_red "================================================================================="
    error_print $error_code
    info_red "The script will exit..."
    info_red "================================================================================="
    exit $error_code
}
