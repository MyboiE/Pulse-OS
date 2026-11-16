#ifndef KERNEL_H
#define KERNEL_H

#define NULL ((void*)0)
#define OS_VERSION "0.1"
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

// Key Navigation Codes
#define KEY_PAGE_UP    0x103
#define KEY_PAGE_DOWN  0x104

// VGA Driver Functions
void vga_clear(void);
void vga_putchar(char c);
void vga_print(const char* str);
void vga_set_color(unsigned char fg, unsigned char bg);
void vga_update_cursor(int row, int col);
void vga_enable_cursor(void);
void vga_scroll_up(int lines);
void vga_scroll_down(int lines);
void vga_scroll_reset(void);
int vga_get_scroll_offset(void);
void vga_render(void);

// Serial Driver Functions
int serial_init(void);
void serial_putchar(char c);
char serial_getchar(void);
void serial_print(const char* str);
int serial_has_char(void);

// Keyboard PS/2 Driver Functions
void keyboard_init(void);
int keyboard_poll(void);
int keyboard_getchar(void);

// Unified Input Function (Keyboard PS/2 + Serial)
int input_getchar(void);

// Common Utility Functions
void kprintf(const char* fmt, ...);
void sys_shutdown(void);
void sys_restart(void);

#endif
