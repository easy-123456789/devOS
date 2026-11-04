/* Raspberry Pi 3B+ starts the 64-bit kernel at 0x80000. */
.section .text.boot,"ax"
.global _start
.type _start, %function
_start:
    mrs x0, mpidr_el1
    and x0, x0, #3
    cbnz x0, 2f
    ldr x0, =__bss_start
    ldr x1, =__bss_end
1:
    cmp x0, x1
    b.hs 3f
    str xzr, [x0], #8
    b 1b
3:
    ldr x0, =__stack_top
    mov sp, x0
    bl kernel_main
2:
    wfe
    b 2b
.size _start, . - _start
