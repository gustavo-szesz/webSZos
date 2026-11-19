ENTRY(_start)

SECTIONS
{
    . = 0x100000;
    _kernel_start = .;

    .text   : { *(.multiboot) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data   : { *(.data .data.*) }
    _kernel_edata = .;

    .bss    : { *(COMMON) *(.bss .bss.*) }
    _kernel_end = .;
}
