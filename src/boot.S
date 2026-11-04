
.section ".text.boot"
.global _start

_start:
    // Keep core 0 alive and halt all others.
    mrs     x0, mpidr_el1
    and     x0, x0, #3
    cbnz    x0, .halt

    // Zero the BSS before entering C.
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end

1:
    cmp     x0, x1
    b.hs    2f
    str     xzr, [x0], #8
    b       1b

2:
    // Set a basic stack.
    mov     sp, #0x80000

    // Jump to the kernel entry point.
    bl      kernel_main

.halt:
    wfe
    b       .halt
