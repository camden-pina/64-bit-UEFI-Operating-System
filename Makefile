MAKE_DIR = $(PWD)

BOOT_DIR := $(MAKE_DIR)

INC_SRCH_PATH :=
INC_SRCH_PATH += -I $(BOOT_DIR)

LIB_SRCH_PATH :=
LIB_SRCH_PATH += -L $(MAKE_FIR)/ext_lib/gnu-efi

CXX = x86_64-mingw32-w64-gcc
LD = ld

CFLAGS
LDFLAGS

all:
	@$(MAKE) -C boot -f boot.mk
