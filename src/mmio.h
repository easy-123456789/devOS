#ifndef MMIO_H
#define MMIO_H
#include <stdint.h>
static inline void mmio_write32(uintptr_t reg, uint32_t value) { *(volatile uint32_t *)reg = value; }
static inline uint32_t mmio_read32(uintptr_t reg) { return *(volatile uint32_t *)reg; }
#endif
