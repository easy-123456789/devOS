#include "command.h"
#include "framebuffer.h"
#include "mmio.h"
#include "picture.h"
#include "uart.h"

#include <stdint.h>

#define LINE_CAPACITY 80u

#define PM_BASE                     0x3F100000u
#define PM_RSTC                     (PM_BASE + 0x1Cu)
#define PM_WDOG                     (PM_BASE + 0x24u)
#define PM_PASSWORD                 0x5A000000u
#define PM_RSTC_WRCFG_MASK          (3u << 4)
#define PM_RSTC_WRCFG_FULL_RESET    (2u << 4)
#define PM_WDOG_TIMEOUT             10u

extern uint32_t asm_demo(void);

static int is_space(char ch)
{
    return ch == ' ' || ch == '\t';
}

static int command_equals(const char *start, unsigned length, const char *word)
{
    unsigned i = 0;

    while (i < length && word[i] != '\0') {
        if (start[i] != word[i]) {
            return 0;
        }
        ++i;
    }

    return i == length && word[i] == '\0';
}

static void print_help(void)
{
    uart_puts("Commands:\n");
    uart_puts("  help     Show this help\n");
    uart_puts("  status   Show current project status\n");
    uart_puts("  systems  List planned emulator systems\n");
    uart_puts("  image    Show a PNG image from the SD card on the video output\n");
    uart_puts("  asm      Run the assembly demo routine\n");
    uart_puts("  reboot   Restart the emulator shell\n");
}

static void run_image(void)
{
    uint32_t x = (framebuffer_width() - 320u) / 2u;
    uint32_t y = (framebuffer_height() - 240u) / 2u;

    if (!picture_load_sd_png("image.png", x, y)) {
        uart_puts("No image could be rendered from the SD card.\n");
    }
}

static void run_asm_demo(void)
{
    uint32_t result = asm_demo();

    uart_puts("Assembly routine returned ");
    uart_putc((char)('0' + (result % 10u)));
    uart_puts(".\n");
}

static void run_reboot(void)
{
    uint32_t reset_control;

    uart_puts("Rebooting...\n");
    uart_flush();

    /* BCM2837 PM watchdog reset: configure it to reset the full SoC. */
    reset_control = mmio_read32(PM_RSTC);
    reset_control &= ~PM_RSTC_WRCFG_MASK;
    reset_control |= PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET;

    mmio_write32(PM_WDOG, PM_PASSWORD | PM_WDOG_TIMEOUT);
    mmio_write32(PM_RSTC, reset_control);
    __asm__ volatile("dsb sy" ::: "memory");

    /* The watchdog should reset the board; do not return to the shell. */
    for (;;) {
        __asm__ volatile("wfe");
    }
}

static void run_command(const char *line)
{
    unsigned start = 0;
    unsigned end;
    unsigned length;

    while (is_space(line[start])) {
        ++start;
    }

    end = start;
    while (line[end] != '\0' && !is_space(line[end])) {
        ++end;
    }

    length = end - start;

    if (length == 0) {
        return;
    }

    /* This first draft accepts commands without arguments. */
    while (is_space(line[end])) {
        ++end;
    }
    if (line[end] != '\0') {
        uart_puts("This command takes no arguments.\n");
        return;
    }

    if (command_equals(&line[start], length, "help")) {
        print_help();
    } else if (command_equals(&line[start], length, "status")) {
        uart_puts("Kernel: running\n");
        uart_puts("ROM loading: not implemented\n");
        uart_puts("Game emulation: not implemented\n");
        uart_puts("GBA cartridge-header checker: included\n");
    } else if (command_equals(&line[start], length, "systems")) {
        uart_puts("Game Boy: planned\n");
        uart_puts("Game Boy Color: planned\n");
        uart_puts("Game Boy Advance: header checker only\n");
        uart_puts("Nintendo DS / DSi: prototype plans\n");
    } else if (command_equals(&line[start], length, "image")) {
        run_image();
    } else if (command_equals(&line[start], length, "asm")) {
        run_asm_demo();
    } else if (command_equals(&line[start], length, "reboot")) {
        run_reboot();
    } else {
        uart_puts("Unknown command. Type 'help'.\n");
    }
}

void command_loop(void)
{
    char line[LINE_CAPACITY];
    unsigned length = 0;
    int overflow = 0;
    int ignore_lf = 0;

    uart_puts("\nBare-metal console. Type 'help'.\n");

    for (;;) {
        uart_puts("> ");

        length = 0;
        overflow = 0;

        for (;;) {
            char ch = uart_getc();

            /* Treat CR, LF, and CRLF as Enter. */
            if (ignore_lf) {
                ignore_lf = 0;
                if (ch == '\n') {
                    continue;
                }
            }

            if (ch == '\r' || ch == '\n') {
                uart_putc('\n');
                ignore_lf = (ch == '\r');
                break;
            }

            if (ch == '\b' || ch == 0x7f) {
                if (length > 0) {
                    --length;
                    overflow = 0;
                    uart_puts("\b \b");
                }
                continue;
            }

            /* Ignore control characters other than Enter and Backspace. */
            if ((unsigned char)ch < 0x20u || (unsigned char)ch > 0x7eu) {
                continue;
            }

            if (length + 1u < LINE_CAPACITY) {
                line[length++] = ch;
                uart_putc(ch);
            } else {
                overflow = 1;
                uart_putc('\a');
            }
        }

        line[length] = '\0';

        if (overflow) {
            uart_puts("Input too long; command discarded.\n");
        } else {
            run_command(line);
        }
    }
}
