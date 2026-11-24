BITS 32

section .text
global irq0_entry
extern timer_irq_handler

irq0_entry:
    pusha
    cld
    call timer_irq_handler
    popa
    iretd
