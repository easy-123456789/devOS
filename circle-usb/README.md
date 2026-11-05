# USB keyboard support (Raspberry Pi 3B+)

The repository's default `make` target builds this Circle-based AArch64 firmware. It replaces the prior freestanding C kernel as the default `kernel8.img` output and includes a small USB-keyboard shell, rather than only echoing keys. Circle supports the Pi 3B+'s DWC2 host controller, built-in USB hub, and standard HID keyboards.

## Build and install

Install an AArch64 bare-metal toolchain (`aarch64-none-elf-gcc` and matching binutils), GNU make, and Git. From the repository root run:

```sh
make
```

The Makefile fetches Circle at a pinned revision into `vendor/circle`, configures it for AArch64 on Raspberry Pi 3, builds its libraries, builds the USB shell, and writes the result to the repository-root `kernel8.img`. Subsequent builds reuse that checkout. To update the Circle revision, override `CIRCLE_REF` in the root Makefile or remove the checkout and ready marker before rebuilding.

Copy the generated `kernel8.img` and the repository's `config.txt` to the Pi boot partition, along with the required Raspberry Pi boot firmware files. Back up the existing image first. Connect a standard USB HID keyboard to a USB-A port and boot. The screen should show a prompt; supported commands are `help`, `status`, `systems`, and `reboot`.

The previous freestanding C kernel remains available explicitly with `make legacy`, which produces `kernel8-legacy.img`; it does not include USB support. The Circle USB shell also does not yet include the old C kernel's SD/FAT32 `load` command or emulator features. Restore the old image or build the legacy image if you want to switch back.

Circle is GPL-3.0; its source and license are present in `vendor/circle` after the build setup. USB operation must still be verified on a physical Raspberry Pi 3B+; it cannot be tested in this source-only environment.
