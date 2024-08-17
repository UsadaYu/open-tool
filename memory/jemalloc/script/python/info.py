import sys
import datetime

def info_red(message):
    print(f"\033[0;31m{message}\033[0m")

def info_green(message):
    print(f"\033[0;32m{message}\033[0m")

def info_yellow(message):
    print(f"\033[0;33m{message}\033[0m")

def info_blue(message):
    print(f"\033[0;34m{message}\033[0m")

def error_exit_print(error_code=-1):
    current_time = datetime.datetime.now()
    formatted_time = current_time.strftime("%Y-%m-%d %H:%M:%S")

    info_red("="*81)
    info_red(formatted_time)
    info_red(f"An error occurred with error code: {error_code}")
    info_red("The script will exit...")
    info_red("="*81)
    sys.exit(error_code)
