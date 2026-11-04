#include "uart.h"
#include "mmio.h"

#define GPIO_BASE 0x3F200000UL
#define AUX_BASE  0x3F215000UL

#define GPPUD        (GPIO_BASE + 0x94)
#define GPPUDCLK0    (GPIO_BASE + 0x98)

#define AUX_ENABLES  (AUX_BASE + 0x04)
#define AUX_MU_IO    (AUX_BASE + 0x40)
#define AUX_MU_IER   (AUX_BASE + 0x44)
#define AUX_MU_IIR   (AUX_BASE + 0x48)
#define AUX_MU_LCR   (AUX_BASE + 0x4C)
#define AUX_MU_MCR   (AUX_BASE + 0x50)
#define AUX_MU_LSR   (AUX_BASE + 0x54)
#define AUX_MU_MSR   (AUX_BASE + 0x58)
#define AUX_MU_SCRATCH (AUX_BASE + 0x5C)
#define AUX_MU_CNTL  (AUX_BASE + 0x60)
#define AUX_MU_STAT  (AUX_BASE + 0x64)
#define AUX_MU_BAUD  (AUX_BASE + 0x68)

static inline void delay_cycles(uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        __asm__ volatile("nop");
    }
}

void uart_init(void)
{
    mmio_write32(GPPUD, 0);
    delay_cycles(150);

    mmio_write32(GPPUDCLK0, (1u << 14) | (1u << 15));
    delay_cycles(150);

    mmio_write32(GPPUDCLK0, 0);

    mmio_write32(AUX_ENABLES, 1);
    mmio_write32(AUX_MU_IER, 0);
    mmio_write32(AUX_MU_CNTL, 0);
    mmio_write32(AUX_MU_LCR, 3);
    mmio_write32(AUX_MU_MCR, 0);
    mmio_write32(AUX_MU_IER, 0);
    mmio_write32(AUX_MU_IIR, 0xC6);
    mmio_write32(AUX_MU_BAUD, 270);

    mmio_write32(AUX_MU_CNTL, 3);
}

void uart_putc(char ch)
{
    while ((mmio_read32(AUX_MU_LSR) & 0x20) == 0) {
        __asm__ volatile("nop");
    }
    mmio_write32(AUX_MU_IO, (uint32_t)(unsigned char)ch);
}

void uart_puts(const char *str)
{
    while (*str != '\0') {
        uart_putc(*str++);
    }
}

char uart_getc(void)
{
    while ((mmio_read32(AUX_MU_LSR) & 0x01) == 0) {
        __asm__ volatile("nop");
    }
    return (char)(mmio_read32(AUX_MU_IO) & 0xFF);
}
