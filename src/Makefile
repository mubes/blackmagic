APP_NAME=blackmagic
PROBE_HOST ?= native
PROBELIB ?= probe
OPT_FLAGS ?= -Og
ENABLE_DEBUG ?=

# Subdirectories where support materials live
PLATFORM_DIR        = platforms/$(PROBE_HOST)
PLATFORM_COMMON_DIR = platforms/common

ifneq ($(V), 1)
MAKEFLAGS += --no-print-dir
Q := @
endif

# Where output files and dependancies are kept
OLOC=ofiles
DEPDIR=.deps
ARTIFACTS=artifacts

CFLAGS += -Wall -Wextra -Werror -Wno-char-subscripts \
	-std=gnu99 -g3 -MD -I./target \
	-I. -Iinclude -I$(OLOC)/include -I$(PLATFORM_DIR)

ifeq ($(ENABLE_DEBUG), 1)
CFLAGS += -DENABLE_DEBUG
endif

# Core probe files
PROBELIBSRC =		\
	command.c	\
	crc32.c		\
	exception.c	\
	gdb_main.c	\
	gdb_hostio.c	\
	gdb_packet.c	\
	hex_utils.c	\
	morse.c		\
	remote.c

# Target related files
PROBELIBSRC +=   		\
	target/adiv5.c		\
	target/adiv5_jtagdp.c	\
	target/adiv5_swdp.c	\
	target/cortexa.c	\
	target/cortexm.c	\
	target/efm32.c		\
	target/jtag_devs.c	\
	target/jtag_scan.c	\
	target/lmi.c		\
	target/lpc_common.c	\
	target/lpc11xx.c	\
	target/lpc17xx.c	\
	target/lpc15xx.c	\
	target/lpc43xx.c	\
	target/kinetis.c	\
	target/msp432.c		\
	target/nrf51.c		\
	target/nxpke04.c	\
	target/sam3x.c		\
	target/sam4l.c		\
	target/samd.c		\
	target/samx5x.c		\
	target/stm32f1.c	\
	target/stm32f4.c	\
	target/stm32h7.c	\
	target/stm32l0.c	\
	target/stm32l4.c	\
	target/target.c		\

# These are files to end up in _all_ application executables, but not the library
SRC  =                  \
	main.c		\

# Include configuration for the specific platform
include $(PLATFORM_DIR)/Makefile.inc

CFLAGS += $(OPT_FLAGS)
LDFLAGS += $(OPT_FLAGS)

ifndef TARGET
ifdef PC_HOSTED
TARGET = $(APP_NAME)
else
TARGET = $(APP_NAME).elf
endif
endif

ifdef NO_OWN_LL
SRC +=                            \
	target/jtagtap_generic.c  \
	target/swdptap_generic.c
endif

ifdef PC_HOSTED
CFLAGS += -DPC_HOSTED=1
else
SRC +=                                   \
	$(PLATFORM_COMMON_DIR)/swdptap.c \
	$(PLATFORM_COMMON_DIR)/jtagtap.c
CFLAGS += -DPC_HOSTED=0 -I$(PLATFORM_COMMON_DIR)
endif

# Flags for automatic depedency generation
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$(OLOC)/$*.d

PROBELIBOBJ = $(patsubst %.S,$(OLOC)/%.o,$(patsubst %.c,$(OLOC)/%.o,$(PROBELIBSRC)))
OBJ = $(patsubst %.S,$(OLOC)/%.o,$(patsubst %.c,$(OLOC)/%.o,$(SRC)))

ifndef CONTROLLED_MAKEFILE
$(warning Please do not run this makefile directly, but from the makefile in the main directory above.)
endif

ifndef BUILD_AS_LIB
$(OLOC)/$(TARGET): lib$(PROBELIB) $(OBJ)
	@echo "  LD      $@"
	$(Q)$(CC) $(DEPFLAGS) -o $@ $(OBJ) -L$(OLOC) -l$(PROBELIB) $(LDFLAGS) -l$(PROBELIB)
endif

lib$(PROBELIB): check_build_platform $(OLOC)/include/version.h $(PROBELIBOBJ)
	@echo "  AR      $@"
	$(Q)$(AR) rcs $(OLOC)/lib$(PROBELIB).a  $(PROBELIBOBJ)

$(OBJ) $(PROBELIBOBJ): $(OLOC)/%.o: %.c $(DEPDIR)/$(OLOC)/%.d  | $(DEPDIR) $(OLOC)
	@echo "  CC      $<"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@

%.bin:	%.elf
	@echo "  OBJCOPY $@"
	$(Q)$(OBJCOPY) -O binary $^ $@

%.hex:	%.elf
	@echo "  OBJCOPY $@"
	$(Q)$(OBJCOPY) -O ihex $^ $@

$(DEPDIR):
	$(Q)mkdir -p $(dir $@)

$(DEPDIR)/%.d:
	$(Q)mkdir -p $(dir $@)

$(OLOC): ; $(Q)mkdir -p $@

.PHONY:	clean host_clean all_platforms depclean FORCE

clean:	host_clean
	$(Q)echo "  CLEAN"
	$(Q)$(RM) -rf $(OLOC)

artclean:
	$(Q)$(RM) -rf $(ARTIFACTS)

depclean: clean artclean
	$(Q)$(RM) -rf $(DEPDIR)

all_platforms:
	$(Q)set -e ;\
	mkdir -p $(ARTIFACTS)/$(shell git describe --always) ;\
	echo "<html><body><ul>" > $(ARTIFACTS)/index.html ;\
	for i in platforms/*/Makefile.inc ; do \
		export DIRNAME=`dirname $$i` ;\
		export PROBE_HOST=`basename $$DIRNAME` ;\
		export CFLAGS=-Werror ;\
		echo "Building for hardware platform: $$PROBE_HOST" ;\
		$(MAKE) $(MAKEFLAGS) clean ;\
		$(MAKE) $(MAKEFLAGS);\
		if [ -f $(OLOC)/$(APP_NAME).bin ]; then \
			mv $(OLOC)/$(APP_NAME).bin $(ARTIFACTS)/$(APP_NAME)-$$PROBE_HOST.bin ;\
			echo "<li><a href='$(APP_NAME)-$$PROBE_HOST.bin'>$$PROBE_HOST</a></li>"\
				>> $(ARTIFACTS)/index.html ;\
		fi ;\
		if [ -f $(OLOC)/$(APP_NAME)_dfu.bin ]; then \
			mv $(OLOC)/$(APP_NAME)_dfu.bin $(ARTIFACTS)/$(APP_NAME)_dfu-$$PROBE_HOST.bin ;\
			echo "<li><a href='$(APP_NAME)_dfu-$$PROBE_HOST.bin'>$$PROBE_HOST DFU</a></li>"\
				>> $(ARTIFACTS)/index.html ;\
		fi ;\
	done ;\
	echo "</ul></body></html>" >> $(ARTIFACTS)/index.html ;\
	cp $(ARTIFACTS)/*.bin $(ARTIFACTS)/$(shell git describe --always)

print-%:
	@echo $* is $($*)

# This should really be forced to make sure we don't pick up an old version
# but that means an almost full rebuild for every cycle. Let's compromise
# and only build it if it doesn't exist.
#$(OLOC)/include/version.h: FORCE  | $(DEPDIR) $(OLOC)
$(OLOC)/include/version.h: | $(DEPDIR) $(OLOC)
	$(Q)echo " GIT      include/version.h"
	$(Q)mkdir -p $(dir $@)
	$(Q)echo "#define FIRMWARE_VERSION \"$(shell git describe --always --dirty)\"" > $@

check_build_platform: FORCE | $(DEPDIR) $(OLOC)
	$(Q)echo " CHK      $(PROBE_HOST)"
	@if ls $(OLOC)/.host_* > /dev/null 2>&1; then \
		if [ ! -f "$(OLOC)/.host_$(PROBE_HOST)" ]; then \
			echo "Building for different PROBE_HOST. Please 'make clean' first" ; \
			exit 1 ; \
		fi; \
	fi; \

	$(Q)touch $(OLOC)/.host_$(PROBE_HOST)

DEPFILES := $(SRC:%.c=$(DEPDIR)/$(OLOC)/%.d) $(PROBELIBSRC:%.c=$(DEPDIR)/$(OLOC)/%.d)

$(DEPFILES):

#include $(wildcard $(DEPFILES))
include $(DEPFILES)

