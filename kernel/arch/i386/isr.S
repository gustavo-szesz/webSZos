.section .text
.global idt_flush
idt_flush:
    mov 4(%esp), %eax
    lidt (%eax)
    ret

.global irq0_stub
.extern irq0_handler
irq0_stub:
    pusha
    call irq0_handler
    popa
    iret

.global isr_default_stub
isr_default_stub:
    iret

.section .note.GNU-stack,"",@progbits
