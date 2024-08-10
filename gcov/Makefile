plat		?= x86
gcov		?= true

CROSS		?=
CC			= $(CROSS)gcc

BINDIR_DIR			:= $(CURDIR)/bin/$(plat)
LIB_DIR				:= $(CURDIR)/lib/$(plat)
SRC_FILE_DIR		:= $(CURDIR)/src
COVERAGE_REPORT_DIR	:= $(CURDIR)/coverage_report

CFLAGS		+= -Wall
CFLAGS 		+= -O0 -g

ifeq ($(gcov), true)
CFLAGS	+= --coverage
LDFLAGS	+= --coverage
# CFLAGS	+= -fprofile-arcs -ftest-coverage
# LDFLAGS	+= -fprofile-arcs
endif

C_SOURCE_FILE	:= $(shell find $(SRC_FILE_DIR) -name "*.c" -exec echo {} +)
C_OBJ_FILES		+= $(patsubst $(SRC_FILE_DIR)/%.c,$(CURDIR)/obj/$(plat)/%.o, $(C_SOURCE_FILE))
C_OBJ_DIR		:= $(shell echo $(dir $(C_OBJ_FILES)) | tr ' ' '\n' | sort -u | tr '\n' ' ')

TARGET	:= main

PHONY	:= default
default: mkdir $(TARGET)

PHONY	+= mkdir
mkdir:
	-@mkdir -p $(BINDIR_DIR)
	-@mkdir -p $(C_OBJ_DIR)
	-@mkdir -p $(COVERAGE_REPORT_DIR)

$(TARGET): $(C_OBJ_FILES)
	$(CC) $(LDFLAGS) -o $(@) -Wl,--start-group $(?) -Wl,--end-group
	mv $(@) $(BINDIR_DIR)

$(CURDIR)/obj/$(plat)/%.o: $(SRC_FILE_DIR)/%.c
	$(CC) $(CFLAGS) -o $(@) -c $(?)

PHONY	+= clean
clean:
	rm -rfv $(CURDIR)/bin $(CURDIR)/obj $(CURDIR)/coverage_report/*

.PHONY: $(PHONY)
