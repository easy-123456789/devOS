# devOS — multi-platform kernel

The goal of this branch is a multi-platform OS with separate images for each supported architecture and boot environment. There is no single image that runs on every computer. The supported Raspberry Pi 3B+ (AArch64) build remains separate, and an x86-64 PC prototype is available.

## x86-64 PC prototype

This target boots through GRUB/Multiboot2, requests a graphics framebuffer, and provides a basic framebuffer text shell with `help`, `status`, `systems`, and `reboot`. It polls a PS/2 set-1 keyboard and PS/2 mouse, as well as COM1 serial input. USB HID, storage, FAT32 loading, networking, and emulation are not implemented. See [`platforms/x86_64_uefi/README.md`](platforms/x86_64_uefi/README.md) for requirements and limitations.

Build from the repository root with GNU Make, an `x86_64-elf` toolchain, GRUB PC/EFI modules, and `xorriso` installed:

```sh
python3 scripts/universal_build.py --target x86_64_uefi
```

The ISO is written to `platforms/x86_64_uefi/build/devos-x86_64.iso`. Test it in QEMU with:

```sh
qemu-system-x86_64 -m 256M -cdrom platforms/x86_64_uefi/build/devos-x86_64.iso
```

## Raspberry Pi 3B+

The Pi target remains separate and unchanged. Install GNU Make, Git, and the `aarch64-none-elf` GCC/binutils toolchain, then run `make` from the repository root. This produces `kernel8.img` using Circle's USB host and HID keyboard support. See [`circle-usb/README.md`](circle-usb/README.md). The legacy freestanding Pi console is available with `make legacy`, producing `kernel8-legacy.img`.

The x86-64 prototype has not been validated on physical hardware and does not have feature parity with the Pi target. It is not a universal computer installer.
