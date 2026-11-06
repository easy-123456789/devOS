#include <stdint.h>

#define COM1 0x3F8u

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_init(void)
{
    outb(COM1 + 1, 0x00); /* Disable interrupts. */
    outb(COM1 + 3, 0x80); /* Divisor latch access. */
    outb(COM1 + 0, 0x01); /* 115200 baud. */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); /* 8 data bits, no parity, one stop bit. */
    outb(COM1 + 2, 0xC7); /* Enable and clear FIFO. */
    outb(COM1 + 4, 0x0B);
}

static void serial_putc(char ch)
{
    /* Bound the wait so a missing serial device cannot stall the kernel. */
    for (uint32_t n = 0; n < 1000000u; ++n) {
        if ((inb(COM1 + 5) & 0x20u) != 0u) {
            outb(COM1, (uint8_t)ch);
            return;
        }
    }
}

static void serial_write(const char *text)
{
    while (*text != '\0') {
        if (*text == '\n') serial_putc('\r');
        serial_putc(*text++);
    }
}

static void serial_hex32(uint32_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    serial_write("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        serial_putc(digits[(value >> (unsigned)shift) & 0xFu]);
    }
}

void kernel_main(uint32_t boot_magic, uint32_t multiboot_info)
{
    serial_init();
    serial_write("devOS x86-64 kernel prototype\n");
    if (boot_magic == 0x36D76289u) {
        serial_write("Multiboot2 boot handoff: OK\n");
        serial_write("Multiboot2 information address: ");
        serial_hex32(multiboot_info);
        serial_write("\n");
    } else {
        serial_write("Warning: unexpected bootloader magic: ");
        serial_hex32(boot_magic);
        serial_write("\n");
    }
    serial_write("Long mode and 1 GiB identity mapping: active\n");
    serial_write("Early bring-up only; storage, graphics, and USB drivers are not implemented.\n");

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
