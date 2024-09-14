CFLAGS += -Wall -g -O3
CFLAGS += -DLOG_MODULE_NAME='"pollux"'
CFLAGS += -DLOG_PRNT_BUF_SIZE=2048

ifeq ($(asan), yes)
CFLAGS += -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address
endif

ifeq ($(gcov), yes)
CFLAGS += --coverage
endif
