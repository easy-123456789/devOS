# Bare-metal emulator project for Raspberry Pi 3B+

TARGET ?= aarch64-none-elf
CC ?= aarch64-linux-gnu-gcc
LD ?= aarch64-linux-gnu-ld
OBJCOPY ?= aarch64-linux-gnu-objcopy
CFLAGS ?= -O2 -ffreestanding -fno-stack-protector -fno-builtin -Wall -Wextra -Werror -nostdlib -mcpu=cortex-a53
LDFLAGS ?= -nostdlib -T linker.ld

SRC := $(wildcard src/*.c) $(wildcard src/*.S)
OBJ := $(SRC:.c=.o)
OBJ := $(OBJ:.S=.o)

all: kernel8.img

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

kernel8.img: $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o kernel8.elf $(OBJ)
	$(OBJCOPY) -O binary kernel8.elf kernel8.img

clean:
	rm -f $(OBJ) kernel8.elf kernel8.img

.PHONY: all clean
