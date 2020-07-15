CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

# We inheret some material here from the generic stm32 implementation
PLATFORM_STM32 = $(PLATFORM_DIR)/../stm32

OPT_FLAGS = -Os
CFLAGS += -mcpu=cortex-m3 -mthumb \
	-DSTM32F1 -DDISCOVERY_SWLINK -I../libopencm3/include \
	-I$(PLATFORM_STM32)
LDFLAGS_BOOT := $(LDFLAGS) --specs=nano.specs -lopencm3_stm32f1 \
	-Wl,-T,$(PLATFORM_STM32)/stlink.ld -nostartfiles -lc\
	-Wl,-Map=$(OLOC)/mapfile -mthumb -mcpu=cortex-m3 -Wl,-gc-sections \
	-L../libopencm3/lib
LDFLAGS = $(LDFLAGS_BOOT) -Wl,-Ttext=0x8002000

ifeq ($(ENABLE_DEBUG), 1)
LDFLAGS += --specs=rdimon.specs
else
LDFLAGS += --specs=nosys.specs
endif

SRC +=                                                           \
                        $(PLATFORM_COMMON_DIR)/cdcacm.c	         \
                        $(PLATFORM_STM32)/serialno.c	         \
                        $(PLATFORM_STM32)/timing_stm32.c	 \
                        $(PLATFORM_STM32)/traceswodecode.c       \
                        $(PLATFORM_STM32)/traceswoasync.c	 \
                        $(PLATFORM_STM32)/gdb_if.c	         \
                        $(PLATFORM_STM32)/usbuart.c 	         \
                        $(PLATFORM_DIR)/platform.c	         \
                        $(PLATFORM_DIR)/platform_common.c        \
                        timing.c

all:	$(OLOC)/$(APP_NAME).bin $(OLOC)/$(APP_NAME)_dfu.bin $(OLOC)/$(APP_NAME)_dfu.hex $(OLOC)/dfu_upgrade.bin $(OLOC)/dfu_upgrade.hex

$(PLATFORM_DIR)/usbdfu.c $(PLATFORM_STM32)/dfucore.c $(PLATFORM_STM32)/dfu_f1.c $(PLATFORM_DIR)/platform_common.c $(PLATFORM_DIR)/dfu_upgrade.c:  $(OLOC)/include/version.h

$(OLOC)/$(APP_NAME)_dfu.elf: $(OLOC)/$(PLATFORM_DIR)/usbdfu.o $(OLOC)/$(PLATFORM_STM32)/dfucore.o $(OLOC)/$(PLATFORM_STM32)/dfu_f1.o $(OLOC)/$(PLATFORM_DIR)/platform_common.o
	@echo "  LD      $@"
	$(Q)$(CC) $^ -o $@ $(LDFLAGS_BOOT)

$(OLOC)/dfu_upgrade.elf:  $(OLOC)/$(PLATFORM_DIR)/dfu_upgrade.o $(OLOC)/$(PLATFORM_STM32)/dfucore.o $(OLOC)/$(PLATFORM_STM32)/dfu_f1.o $(OLOC)/$(PLATFORM_DIR)/platform_common.o
	@echo "  LD      $@"
	$(Q)$(CC) $^ -o $@ $(LDFLAGS)

$(OLOC)/%.o: %.c
	@echo "  CC      $<"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -c $< -o $@


host_clean:
	-$(Q)$(RM) $(OLOC)/$(APP_NAME).bin $(OLOC)/$(APP_NAME)_dfu $(OLOC)/$(APP_NAME)_dfu.bin $(OLOC)/$(APP_NAME)_dfu.hex

