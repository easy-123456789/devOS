// src/sd.c
#include "sd.h"
#include "mmio.h"
#include <stdint.h>

#define SDHOST_BASE 0x3F202000u

#define SDHOST_CMD  (SDHOST_BASE + 0x00)
#define SDHOST_ARG  (SDHOST_BASE + 0x04)
#define SDHOST_TOUT (SDHOST_BASE + 0x08)
#define SDHOST_CDIV (SDHOST_BASE + 0x0C)
#define SDHOST_RSP0 (SDHOST_BASE + 0x10)
#define SDHOST_RSP1 (SDHOST_BASE + 0x14)
#define SDHOST_RSP2 (SDHOST_BASE + 0x18)
#define SDHOST_RSP3 (SDHOST_BASE + 0x1C)
#define SDHOST_HSTS (SDHOST_BASE + 0x20)
#define SDHOST_VDD  (SDHOST_BASE + 0x24)
#define SDHOST_EDM  (SDHOST_BASE + 0x28)
#define SDHOST_HCFG (SDHOST_BASE + 0x2C)
#define SDHOST_BCT  (SDHOST_BASE + 0x30)
#define SDHOST_BLC  (SDHOST_BASE + 0x34)
#define SDHOST_DATA (SDHOST_BASE + 0x40)

static void sd_wait_cmd_done(void)
{
    for (unsigned i = 0; i < 100000; ++i) {
        uint32_t hsts = mmio_read32(SDHOST_HSTS);
        if ((hsts & 0x1u) != 0u) {
            return;
        }
    }
}

static uint32_t sd_send_cmd(uint32_t cmd, uint32_t arg)
{
    mmio_write32(SDHOST_ARG, arg);
    mmio_write32(SDHOST_CMD, cmd);

    sd_wait_cmd_done();

    return mmio_read32(SDHOST_RSP0);
}

bool sd_init(void)
{
    // Reset and clock setup
    // 1) disable controller
    // 2) set clock to ~400kHz
    // 3) send CMD0
    // 4) send CMD8
    // 5) poll ACMD41 until card is ready
    // 6) set bus width to 4-bit
    // 7) set higher clock rate

    // Example init flow:
    // cmd0, cmd8, acmd41 loop, cmd2, cmd3, cmd9, cmd7
    // bus width set with CMD55 + ACMD6

    return true;
}

bool sd_read_block(uint32_t block, void *dst)
{
    uint8_t *p = (uint8_t *)dst;

    // CMD17 READ_SINGLE_BLOCK
    // setup block length 512
    // wait for data token
    // copy 512 bytes
    // read CRC
    (void)block;
    (void)p;
    return true;
}

bool sd_write_block(uint32_t block, const void *src)
{
    const uint8_t *p = (const uint8_t *)src;

    // CMD24 WRITE_BLOCK
    // write data token 0xFE
    // copy 512 bytes
    // send CRC
    // wait for completion
    (void)block;
    (void)p;
    return true;
}
