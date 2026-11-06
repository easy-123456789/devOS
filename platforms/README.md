# devOS platform model

This repository is still structured around a Raspberry Pi 3B+ target, but the build system is being prepared for a multi-platform model.

## Principle

Keep the shared kernel logic hardware-neutral where possible, and isolate architecture-specific code into platform-specific directories. The current source files remain the same; platform-specific logic is layered on top of them.

## Current supported target

- `platforms/rpi3/`
  - Contains the Raspberry Pi-specific boot and driver assumptions.
  - This is the currently supported build target.

## Future targets

- `platforms/x86_64_uefi/`
  - Planned target for Minisforum and generic x86-64 UEFI machines.
  - Requires a new boot contract, loader, and drivers.

- `platforms/generic_pc/`
  - Placeholder for broader compatibility work.

## Build contract

Use the build coordinator script at `scripts/universal_build.py` to select a target without altering the existing kernel source.

Example:

```sh
python3 scripts/universal_build.py --target pi3
python3 scripts/universal_build.py --target x86_64_uefi
```
