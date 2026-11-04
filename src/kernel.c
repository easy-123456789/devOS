#include "console.h"
#include "uart.h"

void console_write(const char *text)
{
    uart_puts(text);
}

void console_putc(char ch)
{
    uart_putc(ch);
}

void console_writeln(const char *text)
{
    uart_puts(text);
    uart_putc('\n');
}
