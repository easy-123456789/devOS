# Bare-metal Raspberry Pi 3B+ screen console

This is the first bootable milestone: it starts directly on a Raspberry Pi 3B+ and draws a text console to an HDMI display using the firmware framebuffer mailbox. It is not yet a game emulator and does not include ROMs, BIOS files, or third-party libraries.

## Build

Install an AArch64 bare-metal toolchain (`aarch64-none-elf-gcc` and matching binutils), then run:

```sh
make clean
make
```

This produces `kernel8.img` and `config.txt`. Copy both files to the root of a FAT32-formatted SD card containing the Raspberry Pi 3B+ boot firmware files (`start.elf` and `fixup.dat`), then boot the Pi with an HDMI display attached.

## Loading a file from the SD card

The read-only EMMC driver supports SDHC/SDXC and standard-capacity cards using single-sector reads. Format a card as FAT32, place an 8.3-named file in the root or a subdirectory, then at the UART shell run:

```text
load ASM/HELLO.S
```

The loader mounts a FAT32 superfloppy or primary MBR FAT32 partition, follows FAT cluster chains, and copies a file (up to 64 KiB) into a kernel buffer. Path components must use 8.3 names. This is file loading only: `.S` text is not assembled at runtime and loaded contents are not executed. The SD driver has not been validated on physical Pi hardware in this environment; verify card detection, clocking, and CMD/data behavior on the target board.

## Current scope

- AArch64 boot entry, BSS initialization, and stack setup
- Raspberry Pi 3 mailbox framebuffer request (640×480, 32-bit)
- Minimal built-in text renderer and onscreen console
- Read-only SD block access and basic FAT32 8.3 file loading
- No OS, runtime library, ROM loader, or emulator core yet

The Pi 3B+ is the target hardware. GB/GBC/GBA emulation and then DS/DSi support are future work; DS/DSi performance on this hardware is not guaranteed. Use only game and firmware data you are legally authorized to use.

## Files

- `src/boot.S`: kernel entry point
- `src/kernel.c`: startup screen
- `src/framebuffer.c`, `src/framebuffer.h`: mailbox framebuffer and drawing
- `src/sd.c`, `src/sd.h`: EMMC block reader
- `src/fat32.c`, `src/fat32.h`: FAT32 mount and file loading
- `src/console.c`, `src/console.h`: screen text console
- `Makefile`, `linker.ld`, `config.txt`: build and Pi boot configuration
