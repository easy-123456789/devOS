# devOS x86-64 PC prototype

The x86-64 target is a separate GRUB/Multiboot2 PC kernel. It requests a graphics framebuffer and provides a basic framebuffer text shell with `help`, `status`, `systems`, and `reboot` commands. It polls for a PS/2 set-1 keyboard, a PS/2 mouse, and COM1 serial input. The mouse pointer is drawn on the framebuffer and left-click is highlighted.

Input support is limited to legacy PS/2-compatible devices and serial; USB HID keyboards and mice are not implemented. Hardware that has no PS/2 controller/device may not provide keyboard or mouse input to this prototype. It is not feature parity with the Pi target: USB host support, SD/MMC, FAT32 file loading, graphics acceleration, networking, and game emulation are not implemented. Framebuffer display requires GRUB to provide a direct RGB framebuffer within the first 4 GiB mapped by the bootstrap.

## Requirements

- An `x86_64-elf` GCC/binutils toolchain (or a compatible compiler prefix)
- GNU Make
- `grub-mkrescue`, GRUB PC/EFI modules, and `xorriso` to build an ISO
- QEMU (`qemu-system-x86_64`) for recommended testing

On Debian/Ubuntu, package names vary by release; typical packages include `grub-pc-bin`, `grub-efi-amd64-bin`, `xorriso`, and `qemu-system-x86`.

## Build

From the repository root:

```sh
python3 scripts/universal_build.py --target x86_64_uefi
```

Or build directly with `make -C platforms/x86_64_uefi`. The output is `platforms/x86_64_uefi/build/devos-x86_64.iso`.

## Test in QEMU

```sh
qemu-system-x86_64 -m 256M -cdrom platforms/x86_64_uefi/build/devos-x86_64.iso
```

For serial input/output instead of the graphical window:

```sh
qemu-system-x86_64 -m 256M -cdrom platforms/x86_64_uefi/build/devos-x86_64.iso -serial stdio -display none
```

Use a disposable VM or test machine; this prototype does not install to a disk or modify storage. The GRUB ISO may be bootable on BIOS or UEFI systems depending on installed GRUB platform modules; this is not a UEFI-native EFI application.
