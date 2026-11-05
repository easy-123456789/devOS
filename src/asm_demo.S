/* A safe, statically linked demo routine callable by the shell. */
.section .text,"ax"
.align 2
.global asm_demo
.type asm_demo, %function
asm_demo:
    mov w0, #42
    ret
.size asm_demo, . - asm_demo
