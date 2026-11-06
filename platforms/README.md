# Platform isolation model

This repository is organized around a simple rule: keep the Raspberry Pi code isolated, and build any future universal support as platform-specific layers on top of shared logic.

## Current platform

- `platforms/rpi3/` : the current supported target. The Raspberry Pi 3B+ code and build assumptions remain here.

## Planned platform

- `platforms/x86_64_uefi/` : future target for Minisforum and generic x86-64 UEFI PCs. This is intentionally separate from the Pi code and will require a different boot contract.

## Shared code policy

Place only hardware-neutral logic in the top-level shared code. Anything that depends on Pi GPIO, the ARM boot flow, Circle, or the Pi memory map stays inside the Pi platform directory. When a future port is developed, it will have its own targeted startup, driver, and build files, rather than modifying the existing Pi source.

## Build entry point

Use the build coordinator script to select a target without changing the existing kernel source files:

```sh
python3 scripts/universal_build.py --target pi3
python3 scripts/universal_build.py --target x86_64_uefi
```
