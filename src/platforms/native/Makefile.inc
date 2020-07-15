CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

# We inheret some material here from the generic stm32 implementation
PLATFORM_STM32 = $(PLATFORM_DIR)/../stm32

CFLAGS += -Istm32/include -mcpu=cortex-m3 -mthumb \
	-DSTM32F1 -DBLACKMAGIC -I../libopencm3/include \
	-I$(PLATFORM_STM32)

LDFLAGS_BOOT := $(LDFLAGS) --specs=nano.specs -lopencm3_stm32f1 \
	-Wl,-T,$(PLATFORM_STM32)/blackmagic.ld -nostartfiles -lc \
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
                        $(PLATFORM_STM32)/traceswo.c	         \
                        $(PLATFORM_STM32)/gdb_if.c	         \
                        $(PLATFORM_STM32)/usbuart.c 	         \
                        $(PLATFORM_DIR)/platform.c	         \
                        timing.c

all:	$(OLOC)/$(APP_NAME).bin $(OLOC)/$(APP_NAME)_dfu.bin $(OLOC)/$(APP_NAME)_dfu.hex

$(OLOC)/$(APP_NAME)_dfu.elf: $(OLOC)/$(PLATFORM_DIR)/usbdfu.o $(OLOC)/$(PLATFORM_STM32)/dfucore.o $(OLOC)/$(PLATFORM_STM32)/dfu_f1.o
	@echo "  LD      $@"
	$(Q)$(CC) $^ -o $@ $(LDFLAGS_BOOT)

$(OLOC)/%.o: %.c
	@echo "  CC      $<"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

host_clean:
	-$(Q)$(RM) $(OLOC)/$(APP_NAME).bin $(OLOC)/$(APP_NAME)_dfu $(OLOC)/$(APP_NAME)_dfu.bin $(OLOC)/$(APP_NAME)_dfu.hex
