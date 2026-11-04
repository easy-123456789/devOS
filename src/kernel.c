#include "console.h"
#include "framebuffer.h"

void kernel_main(void)
{
    if (!framebuffer_init()) {
        for (;;) {
            __asm__ volatile("wfe");
        }
    }

    console_init();
    console_write("RPI 3B+  |  BARE-METAL CONSOLE");
    console_newline();
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
    console_write("[--] Emulator cores are not implemented yet");
    console_newline();
    console_newline();
    console_write("Next: input, SD storage, then GB/GBC core.");

    for (;;) {
        __asm__ volatile("wfe");
    }
}
