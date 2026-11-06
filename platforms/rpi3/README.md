# Raspberry Pi 3B+ platform

This is the current supported platform. The Raspberry Pi-specific assumptions live here so they do not contaminate the broader project.

## Scope

- AArch64 boot flow
- Pi memory map and registers
- Circle USB support
- Raspberry Pi SD card and GPIO assumptions

## Policy

The kernel source remains the same as before. The Pi-specific platform code stays isolated so the rest of the project can evolve toward broader machine support without rewriting the core OS logic.
