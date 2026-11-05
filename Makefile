# Circle-based USB HID firmware is the default output for this repository.
# Circle itself is fetched into vendor/ and is not committed here.
CIRCLE_REF ?= 7a81e1b480fe7d19fc1fc695b89bff0a0848f44b
CIRCLE_DIR := vendor/circle
CIRCLE_READY := $(CIRCLE_DIR)/.usb-keyboard-ready

CROSS_COMPILE ?= aarch64-none-elf-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
CFLAGS = -O2 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -mgeneral-regs-only -mcpu=cortex-a53
LDFLAGS = -nostdlib -T linker.ld -Wl,--build-id=none

C_SOURCES := $(wildcard src/*.c)
ASM_SOURCES := $(wildcard src/*.S)
OBJECTS := $(C_SOURCES:.c=.o) $(ASM_SOURCES:.S=.o)

.PHONY: all setup-usb usb legacy clean

# `make` now produces the USB-keyboard-enabled Circle firmware image.
all: kernel8.img

setup-usb: $(CIRCLE_READY)

$(CIRCLE_DIR):
	git clone https://github.com/rsta2/circle.git $@

$(CIRCLE_READY): | $(CIRCLE_DIR)
	cd $(CIRCLE_DIR) && git checkout --detach $(CIRCLE_REF)
	printf 'AARCH = 64\nRASPPI = 3\nPREFIX64 = %s\n' '$(CROSS_COMPILE)' > $(CIRCLE_DIR)/Config.mk
	cd $(CIRCLE_DIR) && ./makeall
	touch $@

circle-usb/kernel8.img: $(CIRCLE_READY) circle-usb/Makefile circle-usb/kernel.cpp circle-usb/kernel.h circle-usb/main.cpp
	$(MAKE) -C circle-usb CIRCLEHOME=../$(CIRCLE_DIR)

kernel8.img: circle-usb/kernel8.img
	cp $< $@

usb: kernel8.img

# Keep the original freestanding C console available as an explicitly named build.
legacy: kernel8-legacy.img

kernel8-legacy.elf: $(OBJECTS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) -lgcc

kernel8-legacy.img: kernel8-legacy.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -std=c11 -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) kernel8.elf kernel8.img kernel8-legacy.elf kernel8-legacy.img
	$(MAKE) -C circle-usb clean CIRCLEHOME=../$(CIRCLE_DIR) 2>/dev/null || true

