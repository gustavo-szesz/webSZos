ENTRY(_start)
SECTIONS {
    . = 0x100000;
    _kernel_start = .;
    .multiboot : ALIGN(4) { KEEP(*(.multiboot)) }
    .text : ALIGN(4) { *(.text*) }
    .rodata : ALIGN(4) { *(.rodata*) }
    .data : ALIGN(4) { *(.data*) }
    _kernel_edata = .;
    .bss : ALIGN(4) { *(COMMON) *(.bss*) }
    _kernel_end = .;
}