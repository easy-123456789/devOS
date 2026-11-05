# Bare-metal Raspberry Pi 3B+ screen console

This is a bootable AArch64 bare-metal console for Raspberry Pi 3B+. The default `make` target builds the existing C kernel. It accepts shell input through UART; its current firmware does not include USB host/HID keyboard support.

To build the alternate Circle-based USB keyboard firmware, see [`circle-usb/README.md`](circle-usb/README.md). That build supports a directly connected USB HID keyboard through the Pi's USB host controller and hub, and echoes keypresses to HDMI. It replaces the existing console image while installed; restore the original `kernel8.img` to return to the UART shell.

## Build the original C console

Install an AArch64 bare-metal toolchain (`aarch64-none-elf-gcc` and matching binutils), then run:

```sh
make clean
make
```

This produces `kernel8.img` and `config.txt`.

## Build the USB keyboard firmware

Follow the separate build and install instructions in `circle-usb/README.md`. Circle must be cloned into `vendor/circle` and its libraries built first. The alternate firmware is tested only when run on Raspberry Pi hardware; it is not built by default.
