ifneq ($(V), 1)
MFLAGS += --no-print-dir
Q := @
endif

ifeq ($(PROBE_HOST), libftdi)
	PC_HOSTED = true
endif
ifeq ($(PROBE_HOST), pc-stlinkv2)
	PC_HOSTED = true
endif
ifeq ($(PROBE_HOST), hosted)
	PC_HOSTED = true
endif
ifeq ($(PROBE_HOST), library)
	PC_HOSTED = true
endif

ifndef PC_HOSTED
USE_CM3=cm3
endif

.PHONY: cm3

all: $(USE_CM3)
	$(Q) CONTROLLED_MAKEFILE=1 $(MAKE) $(MFLAGS) -C src

cm3:
	$(Q)if [ ! -f libopencm3/Makefile ]; then \
		echo "Initialising git submodules..." ;\
		git submodule init ;\
		git submodule update ;\
	fi
	$(Q)$(MAKE) $(MFLAGS) -C libopencm3 lib

all_platforms: cm3
	$(Q) CONTROLLED_MAKEFILE=1 $(MAKE) $(MFLAGS) -C src $@

cm3_clean:
	$(Q)$(MAKE) $(MFLAGS) -C libopencm3 $@

clean:
	$(Q) CONTROLLED_MAKEFILE=1 $(MAKE) $(MFLAGS) -C src $@
