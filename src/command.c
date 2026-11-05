#include "command.h"
#include "fat32.h"
#include "mmio.h"
#include "sd.h"
#include "uart.h"

#include <stddef.h>
#include <stdint.h>

#define LINE_CAPACITY 80u
#define FILE_BUFFER_CAPACITY (64u * 1024u)

#define PM_BASE                     0x3F100000u
#define PM_RSTC                     (PM_BASE + 0x1Cu)
#define PM_WDOG                     (PM_BASE + 0x24u)
#define PM_PASSWORD                 0x5A000000u
#define PM_RSTC_WRCFG_MASK          (3u << 4)
#define PM_RSTC_WRCFG_FULL_RESET    (2u << 4)
#define PM_WDOG_TIMEOUT             10u

static uint8_t file_buffer[FILE_BUFFER_CAPACITY];

static int is_space(char ch)
{
    return ch == ' ' || ch == '\t';
}

static int command_equals(const char *start, unsigned length, const char *word)
{
    unsigned i = 0;
    while (i < length && word[i] != '\0') {
        if (start[i] != word[i]) return 0;
        ++i;
    }
    return i == length && word[i] == '\0';
}

static void print_help(void)
{
    uart_puts("Commands:\n");
    uart_puts("  help          Show this help\n");
    uart_puts("  status        Show current project status\n");
    uart_puts("  systems       List planned emulator systems\n");
    uart_puts("  load <path>   Load a FAT32 file into RAM (8.3 names)\n");
    uart_puts("  reboot        Restart the emulator shell\n");
}

static void run_load(const char *path)
{
    struct fat32_volume volume;
    size_t size = 0u;

    if (!fat32_mount(&volume)) {
        uart_puts("SD card/FAT32 mount failed.\n");
        return;
    }
    if (!fat32_load_file(&volume, path, file_buffer, sizeof(file_buffer), &size)) {
        uart_puts("File could not be loaded (check path, 8.3 names, and 64 KiB limit).\n");
        return;
    }
    uart_puts("Loaded ");
    /* Print a bounded decimal size without requiring libc. */
    {
        char digits[10];
        unsigned n = 0u;
        size_t value = size;
        if (value == 0u) digits[n++] = '0';
        while (value != 0u && n < sizeof(digits)) {
            digits[n++] = (char)('0' + (value % 10u));
            value /= 10u;
        }
        while (n != 0u) uart_putc(digits[--n]);
    }
    uart_puts(" bytes into RAM. File contents are not executed.\n");
}

static void run_reboot(void)
{
    uint32_t reset_control;
    uart_puts("Rebooting...\n");
    uart_flush();
    reset_control = mmio_read32(PM_RSTC);
    reset_control &= ~PM_RSTC_WRCFG_MASK;
    reset_control |= PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET;
    mmio_write32(PM_WDOG, PM_PASSWORD | PM_WDOG_TIMEOUT);
    mmio_write32(PM_RSTC, reset_control);
    __asm__ volatile("dsb sy" ::: "memory");
    for (;;) __asm__ volatile("wfe");
}

static void run_command(const char *line)
{
    unsigned start = 0u, end, length;
    while (is_space(line[start])) ++start;
    end = start;
    while (line[end] != '\0' && !is_space(line[end])) ++end;
    length = end - start;
    if (length == 0u) return;
    while (is_space(line[end])) ++end;

    if (command_equals(&line[start], length, "load")) {
        unsigned path_start = end, path_end;
        if (line[path_start] == '\0') {
            uart_puts("Usage: load <path> (for example ASM/HELLO.S)\n");
            return;
        }
        path_end = path_start;
        while (line[path_end] != '\0' && !is_space(line[path_end])) ++path_end;
        end = path_end;
        while (is_space(line[end])) ++end;
        if (line[end] != '\0') {
            uart_puts("Usage: load <path>\n");
            return;
        }
        run_load(&line[path_start]);
        return;
    }

    if (line[end] != '\0') {
        uart_puts("This command takes no arguments.\n");
        return;
    }
    if (command_equals(&line[start], length, "help")) {
        print_help();
    } else if (command_equals(&line[start], length, "status")) {
        uart_puts("Kernel: running\nROM loading: not implemented\nGame emulation: not implemented\n");
    } else if (command_equals(&line[start], length, "systems")) {
        uart_puts("Game Boy: planned\nGame Boy Color: planned\nGame Boy Advance: header checker only\n");
        uart_puts("Nintendo DS / DSi: prototype plans\n");
    } else if (command_equals(&line[start], length, "reboot")) {
        run_reboot();
    } else {
        uart_puts("Unknown command. Type 'help'.\n");
    }
}

void command_loop(void)
{
    char line[LINE_CAPACITY];
    unsigned length = 0u;
    int overflow = 0;
    int ignore_lf = 0;
    uart_puts("\nBare-metal console. Type 'help'.\n");
    for (;;) {
        uart_puts("> ");
        length = 0u;
        overflow = 0;
        for (;;) {
            char ch = uart_getc();
            if (ignore_lf) {
                ignore_lf = 0;
                if (ch == '\n') continue;
            }
            if (ch == '\r' || ch == '\n') {
                uart_putc('\n');
                ignore_lf = (ch == '\r');
                break;
            }
            if (ch == '\b' || ch == 0x7f) {
                if (length > 0u) { --length; overflow = 0; uart_puts("\b \b"); }
                continue;
            }
            if ((unsigned char)ch < 0x20u || (unsigned char)ch > 0x7eu) continue;
            if (length + 1u < LINE_CAPACITY) { line[length++] = ch; uart_putc(ch); }
            else { overflow = 1; uart_putc('\a'); }
        }
        line[length] = '\0';
        if (overflow) uart_puts("Input too long; command discarded.\n");
        else run_command(line);
    }
}
