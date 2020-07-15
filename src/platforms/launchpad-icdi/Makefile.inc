CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

PLATFORM_TM4C = $(PLATFORM_DIR)/../tm4c

INCLUDES = -I../libopencm3/include

CPU_FLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS += $(INCLUDES) $(CPU_FLAGS) -DTARGET_IS_BLIZZARD_RB1 -DLM4F -DPART_TM4C123GH6PM

LINKER_SCRIPT="$(PLATFORM_TM4C)/tm4c.ld"
LDFLAGS = -nostartfiles -lc $(CPU_FLAGS) -nodefaultlibs -T$(LINKER_SCRIPT) -Wl,--gc-sections \
	-L../libopencm3/lib -lopencm3_lm4f -lnosys -lm -lgcc
	
SRC +=	$(PLATFORM_COMMON_DIR)/cdcacm.c	\
        $(PLATFORM_DIR)/platform.c      \
	$(PLATFORM_TM4C)/usbuart.c	\
	$(PLATFORM_TM4C)/traceswo.c     \
        $(PLATFORM_TM4C)/gdb_if.c       \
	timing.c

all: $(OLOC)/$(APP_NAME).bin

