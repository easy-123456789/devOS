#include "emulator.h"
static const struct emulator_profile profiles[EMU_SYSTEM_COUNT] = {
    { EMU_GB, "Game Boy", 8, 0 },
    { EMU_GBC, "Game Boy Color", 8, 0 },
    { EMU_GBA, "Game Boy Advance", 32, 0 },
    { EMU_NDS, "Nintendo DS", 512, 0 },
    { EMU_NDSI, "Nintendo DSi", 512, 0 }
};
const struct emulator_profile *emulator_get_profile(enum emulator_system system)
{
    if ((unsigned)system >= EMU_SYSTEM_COUNT) return 0;
    return &profiles[system];
}
