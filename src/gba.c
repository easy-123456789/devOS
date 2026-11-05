#include "gba.h"

#define GBA_TITLE_OFFSET       0xA0u
#define GBA_GAME_CODE_OFFSET   0xACu
#define GBA_MAKER_CODE_OFFSET  0xB0u
#define GBA_FIXED_BYTE_OFFSET  0xB2u
#define GBA_VERSION_OFFSET     0xBCu
#define GBA_CHECKSUM_OFFSET    0xBDu

static void copy_field(char *destination, const uint8_t *source, uint32_t length)
{
    for (uint32_t i = 0; i < length; ++i) {
        destination[i] = (char)source[i];
    }
    destination[length] = '\0';
}

enum gba_header_status gba_parse_rom_header(
    const uint8_t *rom,
    uint32_t rom_size,
    struct gba_cartridge_info *info)
{
    if (rom == 0 || info == 0) {
        return GBA_HEADER_NULL_ARGUMENT;
    }
    if (rom_size < GBA_ROM_HEADER_SIZE) {
        return GBA_HEADER_TOO_SMALL;
    }
    if (rom[GBA_FIXED_BYTE_OFFSET] != 0x96u) {
        return GBA_HEADER_BAD_FIXED_BYTE;
    }

    /* The GBA header checksum is the low byte of -(0x19 + sum(A0..BC)). */
    uint8_t sum = 0x19u;
    for (uint32_t i = GBA_TITLE_OFFSET; i <= GBA_VERSION_OFFSET; ++i) {
        sum = (uint8_t)(sum + rom[i]);
    }
    if ((uint8_t)(0u - sum) != rom[GBA_CHECKSUM_OFFSET]) {
        return GBA_HEADER_BAD_CHECKSUM;
    }

    copy_field(info->title, &rom[GBA_TITLE_OFFSET], 12u);
    copy_field(info->game_code, &rom[GBA_GAME_CODE_OFFSET], 4u);
    copy_field(info->maker_code, &rom[GBA_MAKER_CODE_OFFSET], 2u);
    info->version = rom[GBA_VERSION_OFFSET];
    return GBA_HEADER_OK;
}
