#include "../kernel/include/kernel.h"

#define PORT 0x3f8 // Port COM1 bawaan PC / QEMU

// Inisialisasi port serial COM1
int serial_init(void) {
    outb(PORT + 1, 0x00);    // Disable interrupts
    outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT + 1, 0x00);    //                  (hi byte)
    outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7);    // Enable FIFO, clear, with 14-byte threshold
    outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    return 0;
}

// Mengecek apakah buffer serial siap dikirim
static int is_transmit_empty(void) {
    return inb(PORT + 5) & 0x20;
}

// Menulis satu karakter ke port serial QEMU
void serial_putchar(char c) {
    while (is_transmit_empty() == 0);
    outb(PORT, c);
}

// Mengecek apakah ada data/input keyboard masuk dari serial QEMU
int serial_has_char(void) {
    return inb(PORT + 5) & 1;
}

// Membaca satu karakter dari serial terminal
char serial_getchar(void) {
    while (serial_has_char() == 0);
    return inb(PORT);
}

// Menampilkan string/teks ke serial terminal QEMU
void serial_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_putchar(str[i]);
    }
}
