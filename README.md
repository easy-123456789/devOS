# devOS — multi-platform kernel

The goal of this branch is to grow devOS into a multi-platform operating system. There is no single image that runs on every computer; each supported architecture and boot environment needs its own startup code and drivers. The current supported build targets the Raspberry Pi 3B+ (AArch64). An early x86-64 PC kernel prototype is now available as a separate target.

## x86-64 PC prototype

This target builds a GRUB/Multiboot2 ISO. It starts an x86-64 kernel in long mode and reports boot status over the COM1 serial port. It is an early bring-up prototype, not a fully usable PC OS: storage, graphics, keyboard, networking, and USB support are not implemented. See [`platforms/x86_64_uefi/README.md`](platforms/x86_64_uefi/README.md) for requirements and testing notes.

Build it from the repository root with GNU Make, an `x86_64-elf` toolchain, `grub-mkrescue`, GRUB PC/EFI modules, and `xorriso` installed:

```sh
python3 scripts/universal_build.py --target x86_64_uefi
```

The ISO is written to `platforms/x86_64_uefi/build/devos-x86_64.iso`. For a QEMU smoke test with serial output:

```sh
qemu-system-x86_64 -m 256M -cdrom platforms/x86_64_uefi/build/devos-x86_64.iso -serial stdio
```

## Raspberry Pi 3B+

The Pi target remains supported and unchanged. Install GNU Make, Git, and the `aarch64-none-elf` GCC/binutils toolchain. From the repository root run:

```sh
make
```

This produces `kernel8.img` using Circle's USB host and HID keyboard support. See [`circle-usb/README.md`](circle-usb/README.md) for setup and installation details. The legacy freestanding Pi console is available with `make legacy`, producing `kernel8-legacy.img`.

These are separate platform images; the Pi image is not suitable for a PC and the x86-64 prototype is not a universal computer installer. See [`docs/portability-roadmap.md`](docs/portability-roadmap.md) for the porting plan.
