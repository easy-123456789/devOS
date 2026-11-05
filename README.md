# Bare-metal Raspberry Pi 3B+ console with USB keyboard

The default build creates an AArch64 bare-metal firmware image for Raspberry Pi 3B+ with USB HID keyboard support, using Circle's DWC2 host controller, built-in hub, and keyboard driver. It provides a small keyboard-driven shell (`help`, `status`, `systems`, `reboot`) on the HDMI display.

## Build the default USB-keyboard firmware

Install `aarch64-none-elf-gcc` and matching binutils, GNU make, and Git. From the repository root run:

```sh
make
```

The build automatically fetches Circle at a pinned revision into `vendor/circle`, builds its libraries, then writes the USB-enabled `kernel8.img` to the repository root. See [`circle-usb/README.md`](circle-usb/README.md) for installation details. Copy `kernel8.img`, `config.txt`, and Raspberry Pi boot firmware files to the SD card's boot partition. Back up the existing `kernel8.img` first; this is a bare-metal firmware image, not a Linux kernel module, and it replaces the currently booted firmware when installed.

## Legacy C console

The original freestanding C UART console can still be built separately:

```sh
make legacy
```

This produces `kernel8-legacy.img`, which does not include USB keyboard support. The Circle USB build does not yet port the previous C kernel's SD/FAT32 loader or emulator features.

USB hardware operation must be verified on a physical Pi 3B+; this project cannot test it in the source-only environment.
