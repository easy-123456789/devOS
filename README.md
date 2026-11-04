# Bare-metal Raspberry Pi 3B+ screen console

This is the first bootable milestone: it starts directly on a Raspberry Pi 3B+ and draws a text console to an HDMI display using the firmware framebuffer mailbox. It is not yet a game emulator and does not include ROMs, BIOS files, or third-party libraries.

## Build

Install an AArch64 bare-metal toolchain (`aarch64-none-elf-gcc` and matching binutils), then run:

```sh
make clean
make
```

This produces `kernel8.img` and `config.txt`. Copy both files to the root of a FAT32-formatted SD card containing the Raspberry Pi 3B+ boot firmware files (`start.elf` and `fixup.dat`), then boot the Pi with an HDMI display attached. A build machine is required; this repository does not build itself on the Pi.

The screen should show a blue title bar and a black console with startup/status lines. If framebuffer allocation fails, the kernel intentionally halts; connect a 3.3V USB-to-TTL serial adapter to GPIO14/GPIO15 (115200 8N1) for diagnostics once serial support is added.

## Current scope

- AArch64 boot entry, BSS initialization, and stack setup
- Raspberry Pi 3 mailbox framebuffer request (640×480, 32-bit)
- Minimal built-in 5×7 text renderer and onscreen console
- No OS, runtime library, ROM loader, or emulator core yet

The Pi 3B+ is the target hardware. GB/GBC/GBA emulation and then DS/DSi support are future work; DS/DSi performance on this hardware is not guaranteed. Use only game and firmware data you are legally authorized to use.

## Files

- `src/boot.S`: kernel entry point
- `src/kernel.c`: startup screen
- `src/framebuffer.c`, `src/framebuffer.h`: mailbox framebuffer and drawing
- `src/font.c`, `src/font.h`: compact built-in glyphs
- `src/console.c`, `src/console.h`: screen text console
- `Makefile`, `linker.ld`, `config.txt`: build and Pi boot configuration
