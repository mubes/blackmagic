SYS = $(shell $(CC) -dumpmachine)
CFLAGS += -DENABLE_DEBUG -DPLATFORM_HAS_DEBUG
CFLAGS +=-I ./target

ifneq (, $(findstring linux, $(SYS)))
SRC += serial_unix.c
CFLAGS += -fsanitize=address
LDFLAGS += -lasan
endif

PC_HOSTED=1
BUILD_AS_LIB=1

