# Raspberry Pi 3B+ Bare-Metal Emulator

This repository is a minimal bare-metal project skeleton for a Raspberry Pi 3B+ that is meant to boot directly without a desktop OS and provide the foundation for emulating:

- Game Boy (GB)
- Game Boy Color (GBC)
- Game Boy Advance (GBA)
- Nintendo DS (NDS)
- Nintendo DSi (NDSi)

This is intentionally a low-level starting point, not a complete ROM-ready emulator. It contains:

- a Raspberry Pi 3B+ AArch64 boot entry
- a minimal serial console
- a simple memory-mapped I/O layer
- a platform/emulator registry for future system cores
- the framework needed to add GB/GBC/GBA/NDS cores later

Important limitations:

- This is not a full ROM loader or legal BIOS ROM bundle.
- NDS/DSi emulation is significantly heavier than GB/GBC/GBA and will require a much more complete emulation engine and a stronger platform than a Pi 3B+ is designed to provide.
- The repository intentionally avoids external libraries and keeps the code self-contained.

## Directory layout

- `src/boot.S` - kernel entry point and early boot code
- `src/kernel.c` - main kernel loop and system startup
- `src/uart.c` / `src/uart.h` - UART serial console for debugging
- `src/console.c` / `src/console.h` - print helpers
- `src/mmio.h` - MMIO helper functions
- `src/emulator.c` / `src/emulator.h` - system profile registry
- `linker.ld` - linker script for the Pi 3B+ boot image
- `Makefile` - cross-build instructions

## Build requirements

You need an ARM cross toolchain, typically one of:

- `aarch64-linux-gnu-gcc`
- `clang --target=aarch64-none-elf`
- `arm-none-eabi-gcc`

Then run:

```bash
make
```

The result is a `kernel8.img` boot image that can be placed on an SD card for a Raspberry Pi 3B+.

## Usage notes

This project is intentionally designed as a framework for further work. The next steps would be:

1. Add robust ROM scan/load code for files stored on external media.
2. Add CPU emulation backends for GB/GBC/GBA.
3. Add graphics pipeline and framebuffer abstractions.
4. Add controller input polling.
5. Add DS/DSi core optimizations or move to stronger hardware.

## Legal note

Use only ROMs and BIOS data that you legally own and are authorized to use. This project does not distribute any ROMs or system files.
