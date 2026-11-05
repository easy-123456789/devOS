#include "command.h"
#include "uart.h"

#define LINE_CAPACITY 80u

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
