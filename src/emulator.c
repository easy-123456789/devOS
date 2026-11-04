#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdint.h>

enum emulator_system {
    EMU_GB,
    EMU_GBC,
    EMU_GBA,
    EMU_NDS,
    EMU_NDSI,
    EMU_SYSTEM_COUNT
};

struct emulator_profile {
    enum emulator_system system;
    const char *name;
    uint32_t rom_header_size;
    uint32_t max_rom_size_mb;
    int is_supported;
};

void emulator_init(void);
const struct emulator_profile *emulator_get_profile(enum emulator_system system);

#endif
