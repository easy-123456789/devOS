# Read-only SD loading

The SD reader currently targets Raspberry Pi 3B+ EMMC with single-block SD reads. `load <path>` mounts a FAT32 superfloppy or primary MBR FAT32 partition and copies an 8.3-named file (root or nested directory) into a fixed 64 KiB RAM buffer. This is read-only; `sd_write_block()` returns false. It does not execute loaded files.

Use a FAT32 card and issue a command such as `load ASM/HELLO.S` from the UART shell. This implementation has not been exercised on physical Pi hardware in this environment; verify card detection, EMMC clocking, command/data timeouts, and partition layout on the target before relying on it.
