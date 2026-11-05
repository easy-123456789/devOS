#include "command.h"
#include "console.h"
#include "framebuffer.h"
#include "uart.h"

void kernel_main(void)
{
    uart_init();

    if (!framebuffer_init()) {
        uart_puts("Framebuffer initialization failed.\n");
        for (;;) {
            __asm__ volatile("wfe");
        }
    }

    console_init();
    console_write("RPI 3B+  |  BARE-METAL CONSOLE");
    console_newline();
    console_write("Booted directly on the Raspberry Pi.");
    console_newline();
    console_write("Display: HDMI framebuffer 640 x 480.");
    console_newline();
    console_newline();
    console_write("PROJECT STATUS");
    console_newline();
    console_write("[OK] Bare-metal kernel started");
    console_newline();
    console_write("[OK] Framebuffer initialized");
    console_newline();
    console_write("[OK] GBA cartridge header checker included");
    console_newline();
    console_write("[--] ROM loading and GBA emulation are not implemented");
    console_newline();

    command_loop();
}
