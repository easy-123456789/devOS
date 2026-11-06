# Multi-platform support plan

A single bare-metal image cannot run on every computer. Boot firmware, CPU architecture, memory layout, and device drivers differ between systems. This repository currently targets the Raspberry Pi 3B+; its current boot code and Circle USB build are not a generic PC boot path.

## Practical meaning of “universal”

Treat this as a multi-target OS: keep shared OS logic where it is genuinely hardware-independent, and build a separate image for each supported platform. A reasonable first expansion is **x86-64 PCs that support UEFI**. That target would cover many Minisforum systems and desktops/laptops, but not every computer. Raspberry Pi support remains a separate AArch64 target.

## Current status

This document is a roadmap, not a claim that a universal image exists. The existing `src/boot.S` is AArch64-specific, and the default Circle build is configured for Raspberry Pi 3. No x86-64 UEFI boot image is implemented here yet. The first implementation milestone should be a separately built and tested x86-64 UEFI target, initially with only the hardware features that can be validated on the intended machines.

## Support and testing policy

For each target, document the CPU architecture, boot requirements, build command, known working hardware, and missing drivers. Test booting on real hardware as well as an emulator where possible. Do not describe a platform as supported based only on compiling its image; verify its boot path and required input/display/storage devices.
