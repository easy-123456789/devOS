# devOS — multi-platform kernel

The goal of this branch is to grow devOS into a multi-platform operating system. **A universal kernel image that runs on any computer does not exist yet.** The current boot code and default build target the Raspberry Pi 3B+ (AArch64); they cannot boot on a typical x86-64 PC or Minisforum system. Hardware and firmware vary, so each supported platform needs a compatible boot path and drivers.

## Build the currently supported target: Raspberry Pi 3B+

### Requirements

- GNU make and Git
- A matching AArch64 bare-metal toolchain: `aarch64-none-elf-gcc` and binutils

From the repository root, build the default Circle-based USB-keyboard image:

```sh
make
```

This fetches Circle at the pinned revision specified in the `Makefile`, builds its libraries, and produces `kernel8.img`. Follow [`circle-usb/README.md`](circle-usb/README.md) for installing and booting the image on the Pi. This target is configured specifically for Raspberry Pi 3; do not use its image as a PC installer.

To build the older freestanding C console instead:

```sh
make legacy
```

This produces `kernel8-legacy.img`, also for the Raspberry Pi target.

## Build a new computer/platform target

There is no x86-64 UEFI target or automatic hardware detection implementation yet, so there is currently no command to build a universal PC kernel. To add a target, first choose the platform and boot contract (for example, x86-64 UEFI), then add its own startup/boot files, linker configuration, toolchain settings, and platform drivers. Keep those separate from the Pi-specific `src/boot.S` and Circle configuration. Add a distinct build target and output filename, and share only code that does not depend on a particular architecture or device.

For every target, document required tools and boot media, and test the image in an emulator and on the intended hardware before calling it supported. “Universal” should mean a collection of explicitly supported targets, not a promise that one image works on every computer. See [`docs/portability-roadmap.md`](docs/portability-roadmap.md) for the porting plan.
