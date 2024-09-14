plat ?= x86
asan ?= no
gcov ?= no

ifeq ($(plat),hi3519dv500)
CROSS := aarch64-linux-gnu-hi3519dv500-v2-
else ifeq ($(plat),sd3403v100)
CROSS := aarch64-himix210-linux-sd3403v100-v1-
endif

BUILD_DIR := $(CURDIR)/build_plat
TEMP_DIR := $(CURDIR)/temp/$(plat)
ARTIFACT_DIR := $(TEMP_DIR)/artifact
LIB_DIR := $(CURDIR)/lib/$(plat)
SRC_DIR := $(CURDIR)/src

CXX = $(CROSS)g++
CC = $(CROSS)gcc
LD = $(CROSS)ld
AR = $(CROSS)ar

include $(BUILD_DIR)/common/include.mk
include $(BUILD_DIR)/common/option.mk

TARGET := libpollux.a

CPP_SOURCE_FILE	+= $(shell find $(SRC_DIR) -name "*.cpp" -exec echo {} +)
C_SOURCE_FILE += $(shell find $(SRC_DIR) -name "*.c" -exec echo {} +)
CPP_OBJ_FILES += $(patsubst $(SRC_DIR)/%.cpp, $(TEMP_DIR)/obj/%.o, $(CPP_SOURCE_FILE))
C_OBJ_FILES += $(patsubst $(SRC_DIR)/%.c,   $(TEMP_DIR)/obj/%.o, $(C_SOURCE_FILE))
CPP_OBJ_DIR := $(strip $(shell echo $(dir $(CPP_OBJ_FILES)) | tr ' ' '\n' | sort -u | tr '\n' ' '))
C_OBJ_DIR := $(strip $(shell echo $(dir $(C_OBJ_FILES)) | tr ' ' '\n' | sort -u | tr '\n' ' '))

include $(BUILD_DIR)/$(plat).mk

PHONY := default
default: mkdir $(TARGET)

PHONY += mkdir
mkdir:
	-@mkdir -p $(ARTIFACT_DIR)
	-@mkdir -p $(CPP_OBJ_DIR)
	-@mkdir -p $(C_OBJ_DIR)

$(TARGET): $(CPP_OBJ_FILES) $(C_OBJ_FILES) $(LIBS)
	@echo Linking project files
	$(LD) -r -o $(C_OBJ_DIR)/pollux.o $^
	$(AR) rcs $@ $(C_OBJ_DIR)/pollux.o
	-@mv $@ $(ARTIFACT_DIR) -v

$(TEMP_DIR)/obj/%.o: $(SRC_DIR)/%.cpp
	@echo Compiling: $^
	@$(CXX) $(CFLAGS) -o $@ -c $<

$(TEMP_DIR)/obj/%.o: $(SRC_DIR)/%.c
	@echo Compiling: $^
	@$(CC) $(CFLAGS) -o $@ -c $<

test_install:
	mkdir -p $(CURDIR)/unittests/lib/$(plat)
	cp -f $(ARTIFACT_DIR)/* $(CURDIR)/unittests/lib/$(plat)
	cp -f $(LIB_DIR)/libsirius.a $(CURDIR)/unittests/lib/$(plat)

	mkdir -p $(CURDIR)/unittests/include/sirius
	cp -f $(CURDIR)/include/sirius/*.h $(CURDIR)/unittests/include/sirius
	mkdir -p $(CURDIR)/unittests/include/pollux
	cp -f $(CURDIR)/include/pollux/*.h $(CURDIR)/unittests/include/pollux

PHONY += clean
clean:
	-@rm $(CURDIR)/temp -rfv

.PHONY: $(PHONY)
