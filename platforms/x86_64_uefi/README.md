# devOS x86-64 PC prototype

The x86-64 target is an early GRUB/Multiboot2 PC kernel, separate from the Raspberry Pi source. It now requests a graphics framebuffer and provides a basic framebuffer text shell, plus COM1 serial output/input and polling PS/2 set-1 keyboard input. The shell implements the Pi USB shell's `help`, `status`, `systems`, and `reboot` commands.

This is not feature parity with all historical Pi code and is not a general-purpose OS. USB HID, SD/MMC, FAT32 file loading, graphics acceleration, networking, and game emulation are not implemented. Framebuffer support depends on GRUB providing a direct RGB framebuffer within the first 4 GiB, mapped during early boot. On real computers, legacy PS/2 and COM1 devices may not be available; QEMU is the primary bring-up target.

## Requirements

- An `x86_64-elf` GCC/binutils cross-toolchain (`gcc`, assembler, and linker)
- GNU Make
- `grub-mkrescue`, GRUB PC and EFI modules, and `xorriso` to build an ISO
- QEMU (`qemu-system-x86_64`) is optional for testing

On Debian/Ubuntu, package names vary by release; typical packages include `grub-pc-bin`, `grub-efi-amd64-bin`, `xorriso`, and `qemu-system-x86`.

## Build

From the repository root:

```sh
python3 scripts/universal_build.py --target x86_64_uefi
```

Or build directly:

```sh
make -C platforms/x86_64_uefi
```

The output is `platforms/x86_64_uefi/build/devos-x86_64.iso`. To build only the ELF kernel, run `make -C platforms/x86_64_uefi kernel`. Override the compiler prefix if needed, e.g. `make -C platforms/x86_64_uefi CROSS_COMPILE=x86_64-linux-gnu-`.

## Test in QEMU

For the graphical shell and a keyboard window:

```sh
qemu-system-x86_64 -m 256M -cdrom platforms/x86_64_uefi/build/devos-x86_64.iso
```

For serial input/output instead:

```sh
qemu-system-x86_64 -m 256M -cdrom platforms/x86_64_uefi/build/devos-x86_64.iso -serial stdio -display none
```

The kernel halts after the shell's `reboot` command if the emulated keyboard controller cannot reset. It does not install to a disk or modify storage. Use a disposable VM or test machine and keep important data backed up. The GRUB ISO may be bootable on BIOS or UEFI systems depending on installed GRUB platform modules; this is not yet a UEFI-native EFI application.
