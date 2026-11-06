# x86-64 UEFI platform (planned)

This directory represents the future PC/Minisforum target. It is intentionally isolated from the current Pi platform and is not yet active.

## Planned work

- UEFI entry-point and boot contract
- x86-64 memory map initialization
- text-mode console and keyboard driver stubs
- platform-specific build files
- validation on supported Minisforum models and generic x86-64 PCs

## Policy

No changes are made to the current Raspberry Pi build or kernel source in order to create this target. The x86-64 port will be built as a separate platform, with the shared OS logic left independent from board-specific code.
