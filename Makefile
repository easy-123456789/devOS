CROSS_COMPILE ?= aarch64-none-elf-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
CFLAGS = -O2 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -mgeneral-regs-only -mcpu=cortex-a53
LDFLAGS = -nostdlib -T linker.ld -Wl,--build-id=none

C_SOURCES := $(wildcard src/*.c)
ASM_SOURCES := $(wildcard src/*.S)
OBJECTS := $(C_SOURCES:.c=.o) $(ASM_SOURCES:.S=.o)

all: kernel8.img

kernel8.elf: $(OBJECTS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) -lgcc

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -std=c11 -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) kernel8.elf kernel8.img

.PHONY: all clean
