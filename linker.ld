ENTRY(_start)

SECTIONS
{
    . = 0x80000;

    .text.boot : { *(.text.boot) }
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }

    . = ALIGN(16);
    __bss_start = .;
    .bss : { *(.bss*) *(COMMON) }
    __bss_end = .;

    . = ALIGN(16);
    __end = .;
}
