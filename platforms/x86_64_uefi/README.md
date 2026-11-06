# x86-64 UEFI target (planned)

This target is not implemented yet and is intentionally marked as planned.

A UEFI-capable PC target will need:

- an x86-64 entry point
- a UEFI-compatible bootloader contract
- a platform-specific memory-map implementation
- display and keyboard driver stubs
- separate build and install instructions

No kernel source rewrite is required to start this work. The universal layer will be added around the existing shared code while keeping the Raspberry Pi code isolated.
