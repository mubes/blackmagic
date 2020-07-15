PC_HOSTED = 1
PLATFORM_PC = $(PLATFORM_DIR)/../pc

SYS = $(shell $(CC) -dumpmachine)
CFLAGS += -DENABLE_DEBUG -DPLATFORM_HAS_DEBUG
CFLAGS +=-I ./target -I$(PLATFORM_PC)

ifneq (, $(findstring linux, $(SYS)))
SRC += $(PLATFORM_PC)/serial_unix.c
CFLAGS += -fsanitize=address
LDFLAGS += -lasan
else ifneq (, $(findstring mingw, $(SYS)))
SRC += 	 $(PLATFORM_PC)/serial_win.c
LDFLAGS += -lws2_32
LDFLAGS += -lsetupapi
else ifneq (, $(findstring cygwin, $(SYS)))
SRC += 	 $(PLATFORM_PC)/serial_win.c
LDFLAGS += -lws2_32
LDFLAGS += -lsetupapi
#https://github.com/dmlc/xgboost/issues/1945 indicates macosx as indicator
else ifneq (filter, macosx darwin, $(SYS)))
LDFLAGS += -lsetupapi
LDFLAGS += hidapi/mac/.libs/libhidapi.a -framework IOKit
LDFLAGS += -framework CoreFoundation hidapi/mac/.libs/libhidapi.a
CFLAGS += -Ihidapi/hidapi
endif

LDFLAGS += -lusb-1.0
CFLAGS += $(shell pkg-config --cflags libftdi1)
LDFLAGS += $(shell pkg-config --libs libftdi1)

ifeq ($(shell pkg-config --exists hidapi-libusb && echo 0), 0)
CFLAGS += $(shell pkg-config --cflags hidapi-libusb)
LDFLAGS += $(shell pkg-config --libs hidapi-libusb)
CFLAGS  +=  -DCMSIS_DAP
SRC += $(PLATFORM_DIR)/cmsis_dap.c $(PLATFORM_DIR)/dap.c
endif

#VPATH += platforms/pc

SRC += $(PLATFORM_PC)/gdb_if.c           $(PLATFORM_PC)/cl_utils.c
SRC += $(PLATFORM_PC)/libusb_utils.c     $(PLATFORM_PC)/utils.c
SRC += $(PLATFORM_DIR)/bmp_remote.c      $(PLATFORM_DIR)/remote_swdptap.c
SRC += $(PLATFORM_DIR)/remote_jtagtap.c  $(PLATFORM_DIR)/ftdi_bmp.c
SRC += $(PLATFORM_DIR)/libftdi_swdptap.c $(PLATFORM_DIR)/libftdi_jtagtap.c
SRC += $(PLATFORM_DIR)/jlink.c           $(PLATFORM_DIR)/jlink_adiv5_swdp.c
SRC += $(PLATFORM_DIR)/jlink_jtagtap.c   $(PLATFORM_DIR)/stlinkv2.c
SRC += $(PLATFORM_DIR)/platform.c
SRC += timing.c