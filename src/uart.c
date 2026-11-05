#include "uart.h"

#define GPIO_BASE       0x3F200000u
#define GPIO_FSEL1      (*(volatile uint32_t *)(GPIO_BASE + 0x04u))
#define GPIO_PUD        (*(volatile uint32_t *)(GPIO_BASE + 0x94u))
#define GPIO_PUDCLK0    (*(volatile uint32_t *)(GPIO_BASE + 0x98u))

#define UART0_BASE      0x3F201000u
#define UART0_DR        (*(volatile uint32_t *)(UART0_BASE + 0x00u))
#define UART0_FR        (*(volatile uint32_t *)(UART0_BASE + 0x18u))
#define UART0_IBRD      (*(volatile uint32_t *)(UART0_BASE + 0x24u))
#define UART0_FBRD      (*(volatile uint32_t *)(UART0_BASE + 0x28u))
#define UART0_LCRH      (*(volatile uint32_t *)(UART0_BASE + 0x2Cu))
#define UART0_CR        (*(volatile uint32_t *)(UART0_BASE + 0x30u))
#define UART0_ICR       (*(volatile uint32_t *)(UART0_BASE + 0x44u))

#define UART_FR_RXFE    (1u << 4)
#define UART_FR_TXFF    (1u << 5)

static void uart_write_raw(char ch)
{
    while (UART0_FR & UART_FR_TXFF) {
        __asm__ volatile("nop");
    }
    UART0_DR = (uint32_t)ch;
}

void uart_init(void)
{
    /* GPIO14 and GPIO15: ALT0, used by PL011 UART0. */
    GPIO_FSEL1 &= ~((7u << 12) | (7u << 15));
    GPIO_FSEL1 |=  ((4u << 12) | (4u << 15));

    /* Disable pulls on GPIO14/15 using the Pi 3 GPIO pull-control sequence. */
    GPIO_PUD = 0;
    for (volatile unsigned i = 0; i < 150; ++i) {
        __asm__ volatile("nop");
    }
    GPIO_PUDCLK0 = (1u << 14) | (1u << 15);
    for (volatile unsigned i = 0; i < 150; ++i) {
        __asm__ volatile("nop");
    }
    GPIO_PUDCLK0 = 0;

    UART0_CR = 0;
    UART0_ICR = 0x7FFu;

    /* 115200 baud assuming the PL011 UART clock is 48 MHz. */
    UART0_IBRD = 26;
    UART0_FBRD = 3;
    UART0_LCRH = (3u << 5) | (1u << 4); /* 8-bit, FIFO enabled */
    UART0_CR = (1u << 0) | (1u << 8) | (1u << 9); /* UART, TX, RX */
}

void uart_putc(char ch)
{
    if (ch == '\n') {
        uart_write_raw('\r');
    }
    uart_write_raw(ch);
}

void uart_puts(const char *str)
{
    while (*str) {
        uart_putc(*str++);
    }
}

char uart_getc(void)
{
    while (UART0_FR & UART_FR_RXFE) {
        __asm__ volatile("nop");
    }
    return (char)(UART0_DR & 0xFFu);
}
