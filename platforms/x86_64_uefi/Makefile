# Build configuration for the x86-64 Multiboot2 kernel.
CROSS_COMPILE ?= x86_64-elf-
CC := $(CROSS_COMPILE)gcc
GRUB_MKRESCUE ?= grub-mkrescue
BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/iso-root
KERNEL := $(BUILD_DIR)/kernel.elf
ISO := $(BUILD_DIR)/devos-x86_64.iso

CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-builtin \
          -fno-stack-protector -fno-pic -fno-pie -m64 -mno-red-zone \
          -mno-mmx -mno-sse -mno-sse2 -mcmodel=large
LDFLAGS := -nostdlib -no-pie -Wl,-T,linker.ld -Wl,--build-id=none \
           -Wl,-z,max-page-size=0x1000

.PHONY: all kernel iso clean check-tools
all: iso
kernel: $(KERNEL)
iso: $(ISO)

check-tools:
	@command -v $(CC) >/dev/null || { echo "Missing compiler: $(CC) (install an x86_64-elf GCC toolchain or set CROSS_COMPILE)" >&2; exit 1; }
	@command -v $(GRUB_MKRESCUE) >/dev/null || { echo "Missing grub-mkrescue (install GRUB EFI/PC tools and xorriso)" >&2; exit 1; }

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/boot.o: boot.S | $(BUILD_DIR) check-tools
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: kernel.c | $(BUILD_DIR) check-tools
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o

$(ISO): $(KERNEL) grub.cfg
	mkdir -p $(ISO_ROOT)/boot/grub
	cp $(KERNEL) $(ISO_ROOT)/boot/kernel.elf
	cp grub.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

clean:
	rm -rf $(BUILD_DIR)
