# GBA groundwork

The kernel now includes a small Game Boy Advance cartridge-header parser in `src/gba.c` and `src/gba.h`. It checks that a supplied ROM buffer is large enough, checks the GBA header's fixed byte and checksum, and extracts the title, game code, maker code, and version.

This is only cartridge-header validation. The current kernel has no SD-card ROM loader and does not yet emulate the GBA CPU, graphics, sound, timers, or controls. The message on the boot screen describes this limitation. Do not expect a game to run yet.

The Makefile automatically compiles `src/gba.c` along with the other C source files. Code that later loads a ROM can call `gba_parse_rom_header()` before attempting to execute it. Use ROMs and other game data only when you have the legal right to do so.
