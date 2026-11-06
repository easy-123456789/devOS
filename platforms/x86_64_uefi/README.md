# devOS x86-64 UEFI / PC prototype

This target is an early x86-64 kernel bring-up, separate from the Raspberry Pi source. GRUB loads the Multiboot2 ELF kernel, which enters 64-bit long mode, creates a 1 GiB identity mapping, and writes boot diagnostics to the PC COM1 serial port. It halts after startup; it is **not yet a general-purpose OS** and has no storage, graphics, keyboard, networking, or USB drivers.

The GRUB ISO can be used with GRUB's BIOS or UEFI image when the required GRUB platform modules are installed. This does not imply support for every PC. The kernel currently expects legacy COM1 I/O for visible diagnostics; QEMU serial output is the easiest way to inspect it.

## Requirements

- An `x86_64-elf` GCC/binutils cross-toolchain (`gcc`, assembler, and linker)
- GNU Make
- `grub-mkrescue`, GRUB PC and EFI modules, and `xorriso`
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

The output is `platforms/x86_64_uefi/build/devos-x86_64.iso`. To build only the ELF kernel, run `make -C platforms/x86_64_uefi kernel`. Override the compiler prefix if needed, for example `make -C platforms/x86_64_uefi CROSS_COMPILE=x86_64-linux-gnu-` (the selected compiler must support freestanding x86-64 output).

## Test in QEMU

```sh
qemu-system-x86_64 -m 256M -cdrom platforms/x86_64_uefi/build/devos-x86_64.iso -serial stdio
```

The kernel prints startup status to the serial terminal and then halts. For UEFI firmware testing, use a UEFI-enabled QEMU configuration (such as OVMF) and the generated ISO. Before installing or booting on a physical PC, use a test machine or virtual machine and keep important data backed up; this prototype does not install to a disk or modify storage.

The Raspberry Pi build remains separate and unchanged. See `../../docs/portability-roadmap.md` for the multi-platform support policy.
