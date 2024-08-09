plat		?= x86
jemalloc	?= true

# 是否让代码泄露，true：有内存泄漏；false：无内存泄漏
is_leak		?= true

# 进程是否停止，对应src目录下的exit或not_exit
process_type	?= not_exit

CROSS		?=
CC			= $(CROSS)gcc

BINDIR_DIR	:= $(CURDIR)/bin/$(plat)
LIB_DIR		:= $(CURDIR)/lib/$(plat)
SRC_FILE_DIR:= $(CURDIR)/src
SHARE_DIR	:= /share/$$USER/jemalloc/$(plat)

CFLAGS		+= -Wall -Werror
CFLAGS 		+= -O0 -g
CFLAGS		+= -I$(CURDIR)/include

LDFLAGS		+= -L$(LIB_DIR)
LDFLAGS		+= -lpthread -ldl -lm -pthread -lstdc++

ifeq ($(is_leak), true)
CFLAGS += -DEX_LEAK_FLAG
endif

ifeq ($(jemalloc), true)
LIBS	:= $(LIB_DIR)/libjemalloc.a
LDFLAGS	+= -ljemalloc
endif

C_SOURCE_FILE	:= $(SRC_FILE_DIR)/common.c
C_SOURCE_FILE	+= $(SRC_FILE_DIR)/$(process_type)/main.c
C_OBJ_FILES		:= $(patsubst $(SRC_FILE_DIR)/%.c, $(CURDIR)/obj/$(plat)/%.o, $(C_SOURCE_FILE))
C_OBJ_DIR		:= $(shell echo $(dir $(C_OBJ_FILES)) | tr ' ' '\n' | sort -u | tr '\n' ' ')

TARGET	:= main

PHONY	:= default
default: mkdir pre $(TARGET)

PHONY	+= mkdir
mkdir:
	-@mkdir -p $(BINDIR_DIR)
	-@mkdir -p $(C_OBJ_DIR)
	-@mkdir -p $(SHARE_DIR)

PHONY	+= pre
pre:
	-@chmod 777 $(SHARE_DIR)

$(TARGET): $(C_OBJ_FILES) $(LIBS)
	$(CC) $(LDFLAGS) -o $(@) -Wl,--start-group $(?) -Wl,--end-group
	-@sudo xcp $(@) $(SHARE_DIR)
	mv $(@) $(BINDIR_DIR)

$(CURDIR)/obj/$(plat)/%.o: $(SRC_FILE_DIR)/%.c
	$(CC) $(CFLAGS) -o $(@) -c $(?)

PHONY	+= clean
clean:
	-@rm -rfv $(CURDIR)/bin $(CURDIR)/obj

.PHONY: $(PHONY)
