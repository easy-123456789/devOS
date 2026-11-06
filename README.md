# devOS — multi-platform kernel

The goal of this branch is a multi-platform OS with separate images for each supported architecture and boot environment. There is no single image that runs on every computer. The current supported build targets Raspberry Pi 3B+ (AArch64); a separate x86-64 PC prototype is available.

## x86-64 PC prototype

The x86-64 target boots through GRUB/Multiboot2, requests a graphics framebuffer, and provides a basic framebuffer console with a `help`, `status`, `systems`, and `reboot` shell. Input is polled from a PS/2 keyboard and COM1 serial. USB, storage, FAT32 loading, networking, and emulation are not implemented. See [`platforms/x86_64_uefi/README.md`](platforms/x86_64_uefi/README.md) for requirements and limitations.

Build from the repository root with GNU Make, an `x86_64-elf` toolchain, GRUB PC/EFI modules, and `xorriso` installed:

```sh
python3 scripts/universal_build.py --target x86_64_uefi
```

The ISO is written to `platforms/x86_64_uefi/build/devos-x86_64.iso`. Test the graphical shell in QEMU with:

```sh
qemu-system-x86_64 -m 256M -cdrom platforms/x86_64_uefi/build/devos-x86_64.iso
```

## Raspberry Pi 3B+

The Pi target remains separate and unchanged. Install GNU Make, Git, and the `aarch64-none-elf` GCC/binutils toolchain. From the repository root run:

```sh
make
```

This produces `kernel8.img` using Circle's USB host and HID keyboard support. See [`circle-usb/README.md`](circle-usb/README.md) for setup and installation details. The legacy freestanding Pi console is available with `make legacy`, producing `kernel8-legacy.img`.

The x86-64 prototype has not been validated on physical hardware and does not have feature parity with the Pi target. It is not a universal computer installer. See [`docs/portability-roadmap.md`](docs/portability-roadmap.md) for the porting plan.
