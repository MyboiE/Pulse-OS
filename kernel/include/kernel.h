#ifndef KERNEL_H
#define KERNEL_H

#define NULL ((void*)0)
typedef unsigned int size_t;

// Port I/O Helpers
static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(unsigned short port, unsigned short val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// VGA Driver Functions
void vga_clear(void);
void vga_putchar(char c);
void vga_print(const char* str);
void vga_set_color(unsigned char fg, unsigned char bg);
void vga_update_cursor(int row, int col);
void vga_enable_cursor(void);

// Serial Driver Functions
int serial_init(void);
void serial_putchar(char c);
char serial_getchar(void);
void serial_print(const char* str);
int serial_has_char(void);

// Keyboard PS/2 Driver Functions
void keyboard_init(void);
char keyboard_poll(void);
char keyboard_getchar(void);

// Unified Input Function (Keyboard PS/2 + Serial)
char input_getchar(void);

// Common Utility Functions
void kprintf(const char* fmt, ...);
void sys_shutdown(void);
void sys_restart(void);

#endif
