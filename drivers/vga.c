#include "../kernel/include/kernel.h"

// Memori video VGA Text Mode dimulai dari alamat 0xB8000
static unsigned short* const VGA_MEMORY = (unsigned short*) 0xB8000;
static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;

static int vga_row = 0;
static int vga_column = 0;
static unsigned char vga_color = 0x07; // Warna teks default: Putih dengan background Hitam

// Update posisi kursor hardware di layar
void vga_update_cursor(int row, int col) {
    unsigned short pos = row * VGA_WIDTH + col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

// Aktifkan kursor hardware VGA (underline cursor)
void vga_enable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 13); // Cursor scanline start: 13
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15); // Cursor scanline end: 15
}

// Menentukan warna teks dan latar belakang
void vga_set_color(unsigned char fg, unsigned char bg) {
    vga_color = fg | (bg << 4);
}

// Scroll seluruh layar ke atas sebanyak 1 baris
static void vga_scroll(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (unsigned short)' ' | ((unsigned short)vga_color << 8);
    }
    vga_row = VGA_HEIGHT - 1;
    vga_column = 0;
}

// Membersihkan seluruh layar VGA
void vga_clear(void) {
    vga_enable_cursor();
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const int index = y * VGA_WIDTH + x;
            VGA_MEMORY[index] = (unsigned short)' ' | ((unsigned short)vga_color << 8);
        }
    }
    vga_row = 0;
    vga_column = 0;
    vga_update_cursor(0, 0);
}

// Menampilkan satu karakter ke layar VGA
void vga_putchar(char c) {
    if (c == '\n') {
        vga_column = 0;
        if (++vga_row >= VGA_HEIGHT) {
            vga_scroll();
        }
        vga_update_cursor(vga_row, vga_column);
        return;
    }

    if (c == '\r') {
        vga_column = 0;
        vga_update_cursor(vga_row, vga_column);
        return;
    }

    if (c == '\b') {
        if (vga_column > 0) {
            vga_column--;
            const int index = vga_row * VGA_WIDTH + vga_column;
            VGA_MEMORY[index] = (unsigned short)' ' | ((unsigned short)vga_color << 8);
            vga_update_cursor(vga_row, vga_column);
        }
        return;
    }

    const int index = vga_row * VGA_WIDTH + vga_column;
    VGA_MEMORY[index] = (unsigned short)c | ((unsigned short)vga_color << 8);

    if (++vga_column >= VGA_WIDTH) {
        vga_column = 0;
        if (++vga_row >= VGA_HEIGHT) {
            vga_scroll();
        }
    }
    vga_update_cursor(vga_row, vga_column);
}

// Menampilkan string/teks ke layar VGA
void vga_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}
