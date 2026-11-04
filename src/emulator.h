#include "console.h"
#include "emulator.h"

extern void uart_init(void);

void kernel_main(void)
{
    uart_init();
    console_writeln("[baremetal] Raspberry Pi 3B+ booted");
    console_writeln("[baremetal] target: GB, GBC, GBA, DS, DSi");
    console_writeln("[baremetal] initializing emulator registry");

    emulator_init();

    console_writeln("[baremetal] ready");

    while (1) {
        __asm__ volatile("wfe");
    }
}
