CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

# We inheret some material here from the generic stm32 implementation
PLATFORM_STM32 = $(PLATFORM_DIR)/../stm32

CFLAGS += -Istm32/include -mcpu=cortex-m4 -mthumb \
	-mfloat-abi=hard -mfpu=fpv4-sp-d16 \
	-DSTM32F4 -DF4DISCOVERY -I../libopencm3/include \
	-I$(PLATFORM_STM32)

LDFLAGS = -lopencm3_stm32f4 \
	-Wl,-T,$(PLATFORM_STM32)/f4discovery.ld -nostartfiles -lc -lnosys \
	-Wl,-Map=$(OLOC)/mapfile -mthumb -mcpu=cortex-m4 -Wl,-gc-sections \
	-mfloat-abi=hard -mfpu=fpv4-sp-d16 \
	-L../libopencm3/lib

SRC +=                                                           \
                        $(PLATFORM_COMMON_DIR)/cdcacm.c	         \
                        $(PLATFORM_DIR)/platform.c	         \
                        $(PLATFORM_STM32)/traceswodecode.c       \
                        $(PLATFORM_STM32)/traceswo.c	         \
                        $(PLATFORM_STM32)/usbuart.c 	         \
                        $(PLATFORM_STM32)/serialno.c	         \
                        $(PLATFORM_STM32)/timing_stm32.c	 \
                        $(PLATFORM_STM32)/gdb_if.c               \
                        timing.c

all:	$(OLOC)/$(APP_NAME).bin

host_clean:
	-$(Q)$(RM) $(APP_NAME).bin
