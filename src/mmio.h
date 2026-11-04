#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>

static inline void mmio_write32(uint32_t reg, uint32_t value)
{
    *(volatile uint32_t *)reg = value;
}

static inline uint32_t mmio_read32(uint32_t reg)
{
    return *(volatile uint32_t *)reg;
}

static inline void mmio_write8(uint32_t reg, uint8_t value)
{
    *(volatile uint8_t *)reg = value;
}

static inline uint8_t mmio_read8(uint32_t reg)
{
    return *(volatile uint8_t *)reg;
}

#endif
