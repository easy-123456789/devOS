#ifndef GBA_H
#define GBA_H

#include <stdint.h>

/* A GBA cartridge header occupies bytes 0xA0 through 0xBF. */
#define GBA_ROM_HEADER_SIZE 0xC0u

enum gba_header_status {
    GBA_HEADER_OK = 0,
    GBA_HEADER_NULL_ARGUMENT,
    GBA_HEADER_TOO_SMALL,
    GBA_HEADER_BAD_FIXED_BYTE,
    GBA_HEADER_BAD_CHECKSUM
};

struct gba_cartridge_info {
    char title[13];
    char game_code[5];
    char maker_code[3];
    uint8_t version;
};

/*
 * Check the basic GBA cartridge header and copy its identifying fields.
 * `rom` must point to `rom_size` readable bytes. This does not execute the ROM.
 */
enum gba_header_status gba_parse_rom_header(
    const uint8_t *rom,
    uint32_t rom_size,
    struct gba_cartridge_info *info);

#endif
