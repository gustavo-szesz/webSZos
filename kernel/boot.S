.set MAGIC,    0x1BADB002
.set FLAGS,    0x00010000
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
mb_header:
.long MAGIC
.long FLAGS
.long CHECKSUM
.long mb_header
.long _kernel_start
.long _kernel_edata
.long _kernel_end
.long _start

.section .bss
.align 16
stack_bottom: .skip 16384
stack_top:

.section .text
.global _start
_start:
    movw $0x3f8, %dx              # canal 2: um byte cru na COM1
    movb $'S', %al
    outb %al, %dx
    mov  $stack_top, %esp
    push %ebx
    call kmain
1:  cli
    hlt
    jmp  1b

.section .note.GNU-stack,"",@progbits