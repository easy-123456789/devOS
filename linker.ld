ENTRY(_start)

SECTIONS
{
    . = 0x80000;
    .text.boot : { KEEP(*(.text.boot)) }
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }

    . = ALIGN(16);
    __bss_start = .;
    .bss : {
        *(.bss*)
        *(COMMON)
        . = ALIGN(16);
        . += 0x10000;
        __stack_top = .;
    }
    __bss_end = .;
    /DISCARD/ : { *(.comment*) *(.note*) *(.eh_frame*) }
}
