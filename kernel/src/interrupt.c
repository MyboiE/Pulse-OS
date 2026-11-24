#include "../include/interrupt.h"
#include "../include/kernel.h"
#include "../include/task.h"

typedef struct __attribute__((packed)) {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char flags;
    unsigned short offset_high;
} idt_entry_t;

typedef struct __attribute__((packed)) {
    unsigned short limit;
    unsigned int base;
} idtr_t;

static idt_entry_t idt[256];
static volatile unsigned int timer_ticks;

extern void irq0_entry(void);

static void idt_set_gate(unsigned int vector, void (*entry)(void)) {
    unsigned int address = (unsigned int)entry;
    idt[vector].offset_low = (unsigned short)(address & 0xffffu);
    idt[vector].selector = 0x08;
    idt[vector].zero = 0;
    idt[vector].flags = 0x8e;
    idt[vector].offset_high = (unsigned short)(address >> 16);
}

void interrupt_init(void) {
    idtr_t idtr;
    __asm__ volatile ("cli");

    for (unsigned int i = 0; i < 256; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].flags = 0;
        idt[i].offset_high = 0;
    }
    idt_set_gate(0x20, irq0_entry);
    idtr.limit = sizeof(idt) - 1;
    idtr.base = (unsigned int)idt;
    __asm__ volatile ("lidt %0" : : "m"(idtr));

    outb(0x20, 0x11);
    outb(0xa0, 0x11);
    outb(0x21, 0x20);
    outb(0xa1, 0x28);
    outb(0x21, 0x04);
    outb(0xa1, 0x02);
    outb(0x21, 0x01);
    outb(0xa1, 0x01);
    outb(0x21, 0xfe);
    outb(0xa1, 0xff);

    timer_ticks = 0;
    outb(0x43, 0x36);
    outb(0x40, 11932 & 0xff);
    outb(0x40, 11932 >> 8);
    __asm__ volatile ("sti");
}

void timer_irq_handler(void) {
    timer_ticks++;
    outb(0x20, 0x20);
    scheduler_tick();
}

unsigned int interrupt_timer_ticks(void) {
    return timer_ticks;
}
