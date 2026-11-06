/*
 * Safe RAM bit-flip demo for AArch64 / Raspberry Pi 3B+.
 *
 * This changes bits only inside ram_prank_buffer, a 64-byte buffer reserved
 * by this object in .bss. It never accepts an address and cannot target the
 * kernel, page tables, peripheral registers, or arbitrary physical RAM.
 *
 * Call ram_prank_flip_demo() from C to perform 64 deterministic pseudo-random
 * bit flips in the private buffer. This is a demonstration, not a hardware
 * fault injector.
 */
.section .text,"ax"
.align 2
.global ram_prank_flip_demo
.type ram_prank_flip_demo, %function
ram_prank_flip_demo:
    ldr     x1, =ram_prank_buffer
    mov     w2, #64                 // exactly 64 changes, bounded
    mov     w3, #0x1ACE             // private PRNG seed

1:
    // xorshift32 PRNG; generate a byte offset and bit number.
    eor     w3, w3, w3, lsl #13
    eor     w3, w3, w3, lsr #17
    eor     w3, w3, w3, lsl #5
    and     w4, w3, #63             // offset is always in [0, 63]
    add     x5, x1, x4
    ldrb    w6, [x5]
    lsr     w7, w3, #6
    and     w7, w7, #7              // bit number is always in [0, 7]
    mov     w8, #1
    lsl     w8, w8, w7
    eor     w6, w6, w8
    strb    w6, [x5]

    subs    w2, w2, #1
    b.ne    1b

    dsb     sy
    ret
.size ram_prank_flip_demo, . - ram_prank_flip_demo

.section .bss,"aw",%nobits
.balign 16
ram_prank_buffer:
    .skip 64
