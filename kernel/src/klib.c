#include "../include/kernel.h"

// Output terpadu (VGA + Serial)
void kputc(char c) {
    vga_putchar(c);
    serial_putchar(c);
}

void kputs(const char* str) {
    if (!str) str = "(null)";
    while (*str) {
        kputc(*str++);
    }
}

// String & Memory Helpers
size_t kstrlen(const char* s) {
    size_t len = 0;
    while (s && s[len]) len++;
    return len;
}

int kstrcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int kstrncmp(const char* a, const char* b, size_t n) {
    while (n && *a && (*a == *b)) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

char* kstrcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* kstrncpy(char* dest, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

void* kmemset(void* dest, int val, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    while (count--) *d++ = (unsigned char)val;
    return dest;
}

void* kmemcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (count--) *d++ = *s++;
    return dest;
}

void* memset(void* dest, int val, size_t count) {
    return kmemset(dest, val, count);
}

void* memcpy(void* dest, const void* src, size_t count) {
    return kmemcpy(dest, src, count);
}

// Format number helper untuk kprintf
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)

static void print_num(unsigned int val, unsigned int base, int is_signed, int is_neg) {
    char buf[32];
    int i = 0;
    const char digits[] = "0123456789ABCDEF";

    if (val == 0) {
        kputc('0');
        return;
    }

    while (val > 0) {
        buf[i++] = digits[val % base];
        val /= base;
    }

    if (is_signed && is_neg) {
        kputc('-');
    }

    while (--i >= 0) {
        kputc(buf[i]);
    }
}

void kprintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            if (fmt[i] == 's') {
                char* s = va_arg(ap, char*);
                kputs(s ? s : "(null)");
            } else if (fmt[i] == 'd' || fmt[i] == 'i') {
                int d = va_arg(ap, int);
                if (d < 0) {
                    print_num((unsigned int)(-d), 10, 1, 1);
                } else {
                    print_num((unsigned int)d, 10, 1, 0);
                }
            } else if (fmt[i] == 'u') {
                unsigned int u = va_arg(ap, unsigned int);
                print_num(u, 10, 0, 0);
            } else if (fmt[i] == 'x' || fmt[i] == 'X') {
                unsigned int x = va_arg(ap, unsigned int);
                print_num(x, 16, 0, 0);
            } else if (fmt[i] == 'c') {
                char c = (char)va_arg(ap, int);
                kputc(c);
            } else if (fmt[i] == '%') {
                kputc('%');
            } else {
                kputc('%');
                kputc(fmt[i]);
            }
        } else {
            kputc(fmt[i]);
        }
    }

    va_end(ap);
}

/* kdelay_ms - busy-wait delay.
   Di QEMU, NOP loop dioptimasi compiler, jadi kita pakai port I/O sebagai
   "fence" yang tidak bisa dioptimasi (port 0x80 = POST diagnostic port,
   tidak berbahaya di x86). Setiap outb ≈ ~1-4 µs di QEMU → 1 ms ≈ 400 outb. */
void kdelay_ms(unsigned int ms) {
    for (unsigned int m = 0; m < ms; m++) {
        for (volatile unsigned int i = 0; i < 400; i++) {
            outb(0x80, 0); /* POST port — safe no-op, tidak bisa di-optimasi */
        }
    }
}

/* ---- Architecture stubs ----
   Implementasi penuh (GDT/IDT) dapat diletakkan di file terpisah nantinya.
   Saat ini berfungsi sebagai placeholder agar kernel_main() dapat dikompilasi. */

void gdt_init(void) {
    /* TODO: Setup Global Descriptor Table (flat 32-bit segments) */
}

void idt_init(void) {
    /* TODO: Setup Interrupt Descriptor Table + PIC remap */
}

void interrupts_enable(void) {
    /* JANGAN panggil 'sti' tanpa IDT yang valid — akan triple fault!
       Keyboard & serial sudah dibaca via polling (tidak butuh interrupt).
       Aktifkan ini hanya setelah IDT benar-benar diimplementasi. */
    /* __asm__ volatile ("sti"); */
    (void)0;
}

