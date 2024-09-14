plat ?= x86
asan ?= no
gcov ?= no

ifeq ($(plat),hi3519dv500)
CROSS := aarch64-linux-gnu-hi3519dv500-v2-
CRYPTO_TOOL	?= $(CURDIR)/ckms/elf_crypto.sh
else ifeq ($(plat),sd3403v100)
CROSS := aarch64-himix210-linux-sd3403v100-v1-
CRYPTO_TOOL	= $(CURDIR)/ckms/ckms_web5.0/elf_crypto.sh
else ifeq ($(plat),x86)
LDFLAGS += -lva -lz -llzma -lbz2 -ldrm -lX11 -lva-drm -lva-x11
CRYPTO_TOOL = echo
endif

LIB_DIR := $(CURDIR)/lib/$(plat)
SRC_DIR := $(CURDIR)/src

CC = $(CROSS)gcc

CFLAGS += -I$(CURDIR)/include/pollux
CFLAGS += -I$(CURDIR)/include/sirius
CFLAGS += -Wall -g -O3

CFLAGS += -DLOG_MODULE_NAME='"pollux_test"'
CFLAGS += -DLOG_PRNT_BUF_SIZE=1024

LDFLAGS += -L$(LIB_DIR)
LDFLAGS += -lpthread -lm -ldl

ifeq ($(asan), yes)
CFLAGS += -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address
LDFLAGS += -fsanitize=address
endif
ifeq ($(gcov), yes)
CFLAGS += --coverage
LDFLAGS += --coverage
endif

TARGET := pollux

PHONY := all
all: $(TARGET)

LIBS += $(LIB_DIR)/libpollux.a
LIBS += $(LIB_DIR)/libsirius.a

$(TARGET): $(SRC_DIR)/test.o $(LIBS)
	$(CC) -o $@ -Wl,--start-group $^ -Wl,--end-group $(LDFLAGS)
	-@mkdir $(CURDIR)/temp
	-@$(CRYPTO_TOOL) $@ $@ $(CROSS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

PHONY	+= clean
clean:
	-@find $(CURDIR)/ \
	\( -name "*.o" -o -name "*.gcda" -o -name "*.gcno" \) \
	-exec rm -rfv {} +
	-@rm -rfv $(CURDIR)/temp $(CURDIR)/$(TARGET)

.PHONY: $(PHONY)
