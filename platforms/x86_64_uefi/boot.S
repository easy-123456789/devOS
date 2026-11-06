/* Multiboot2 entry and 32-bit-to-64-bit bootstrap.
 * GRUB enters in 32-bit protected mode with EAX=magic and EBX=info pointer.
 * The bootstrap maps the first 4 GiB using 2 MiB identity-mapped pages.
 */
.set MB2_HEADER_MAGIC, 0xE85250D6
.set MB2_BOOT_MAGIC,   0x36D76289

.section .multiboot2,"a"
.balign 8
mb2_header_start:
    .long MB2_HEADER_MAGIC
    .long 0
    .long mb2_header_end - mb2_header_start
    .long -(MB2_HEADER_MAGIC + (mb2_header_end - mb2_header_start))
    /* Ask GRUB for a 1024x768x32 framebuffer when available. */
    .short 5
    .short 1                         /* Optional request. */
    .long 20
    .long 1024
    .long 768
    .long 32
    /* Required end tag. */
    .short 0
    .short 0
    .long 8
mb2_header_end:

.section .text.boot,"ax"
.code32
.global _start
.type _start, @function
_start:
    cli
    cld
    movl $boot_magic, %edi
    movl %eax, (%edi)
    movl $boot_info, %edi
    movl %ebx, (%edi)

    /* Four PDPT entries point to four page directories, mapping 4 GiB. */
    movl $(boot_pd0 + 3), %eax
    movl %eax, boot_pdpt
    movl $0, boot_pdpt + 4
    movl $(boot_pd1 + 3), %eax
    movl %eax, boot_pdpt + 8
    movl $0, boot_pdpt + 12
    movl $(boot_pd2 + 3), %eax
    movl %eax, boot_pdpt + 16
    movl $0, boot_pdpt + 20
    movl $(boot_pd3 + 3), %eax
    movl %eax, boot_pdpt + 24
    movl $0, boot_pdpt + 28

    movl $boot_pd0, %edi
    xorl %eax, %eax
    movl $2048, %ecx
1:
    movl %eax, (%edi)
    orl $0x83, (%edi)               /* Present, writable, 2 MiB page. */
    movl $0, 4(%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop 1b

    movl $boot_pml4, %eax
    movl %eax, %cr3
    movl %cr4, %eax
    orl $0x20, %eax                 /* CR4.PAE */
    movl %eax, %cr4

    /* Enable EFER.LME. */
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr

    lgdt boot_gdt_pointer
    movl %cr0, %eax
    orl $0x80000000, %eax           /* CR0.PG */
    movl %eax, %cr0
    ljmp $0x08, $long_mode_start

.code64
long_mode_start:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    xorw %ax, %ax
    movw %ax, %fs
    movw %ax, %gs

    movabs $boot_stack_top, %rsp
    andq $-16, %rsp
    movl boot_magic(%rip), %edi
    movl boot_info(%rip), %esi
    call kernel_main

2:
    cli
    hlt
    jmp 2b
.size _start, . - _start

.balign 8
boot_magic:
    .long 0
boot_info:
    .long 0

.balign 16
boot_gdt:
    .quad 0x0000000000000000
    .quad 0x00AF9A000000FFFF           /* 64-bit code segment. */
    .quad 0x00CF92000000FFFF           /* Data segment. */
boot_gdt_end:
boot_gdt_pointer:
    .word boot_gdt_end - boot_gdt - 1
    .long boot_gdt

.section .bss,"aw",@nobits
.balign 4096
boot_pml4:
    .skip 4096
boot_pdpt:
    .skip 4096
boot_pd0:
    .skip 4096
boot_pd1:
    .skip 4096
boot_pd2:
    .skip 4096
boot_pd3:
    .skip 4096
.balign 16
boot_stack:
    .skip 65536
boot_stack_top:
