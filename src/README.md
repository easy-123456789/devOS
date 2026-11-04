#include "emulator.h"
#include "console.h"

static const struct emulator_profile profiles[EMU_SYSTEM_COUNT] = {
    { EMU_GB,   "Game Boy",       0x100,  8,  1 },
    { EMU_GBC,  "Game Boy Color", 0x100,  8,  1 },
    { EMU_GBA,  "Game Boy Advance", 0x200,  32, 1 },
    { EMU_NDS,  "Nintendo DS",     0x200,  512, 0 },
    { EMU_NDSI, "Nintendo DSi",    0x200,  512, 0 },
};

void emulator_init(void)
{
    console_writeln("[emulator] registry initialized");
    console_writeln("[emulator] GB: supported");
    console_writeln("[emulator] GBC: supported");
    console_writeln("[emulator] GBA: supported");
    console_writeln("[emulator] DS: prototype only");
    console_writeln("[emulator] DSi: prototype only");
}

const struct emulator_profile *emulator_get_profile(enum emulator_system system)
{
    if ((unsigned)system >= EMU_SYSTEM_COUNT) {
        return 0;
    }
    return &profiles[system];
}

