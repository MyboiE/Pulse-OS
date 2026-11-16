#include "../kernel/include/kernel.h"

// Memori video VGA Text Mode dimulai dari alamat 0xB8000
static unsigned short* const VGA_MEMORY = (unsigned short*) 0xB8000;
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_HISTORY_MAX 300

// Buffer layar aktif (25 baris)
static unsigned short vga_screen[VGA_HEIGHT][VGA_WIDTH];

// Buffer riwayat baris yang tergeser ke atas (circular buffer)
static unsigned short vga_history[VGA_HISTORY_MAX][VGA_WIDTH];
static int vga_history_count = 0;
static int vga_history_head = 0;

// Posisi scroll offset (0 = tampilan aktif paling bawah, > 0 = mundur ke atas N baris)
static int vga_scroll_offset = 0;

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

// Render tampilan VGA berdasarkan vga_scroll_offset
void vga_render(void) {
    if (vga_scroll_offset == 0) {
        for (int y = 0; y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                VGA_MEMORY[y * VGA_WIDTH + x] = vga_screen[y][x];
            }
        }
        vga_update_cursor(vga_row, vga_column);
        return;
    }

    int oldest_idx = (vga_history_head - vga_history_count + VGA_HISTORY_MAX) % VGA_HISTORY_MAX;

    for (int y = 0; y < VGA_HEIGHT; y++) {
        int v = (vga_history_count + y) - vga_scroll_offset;
        if (v < 0) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                VGA_MEMORY[y * VGA_WIDTH + x] = (unsigned short)' ' | ((unsigned short)0x07 << 8);
            }
        } else if (v < vga_history_count) {
            int slot = (oldest_idx + v) % VGA_HISTORY_MAX;
            for (int x = 0; x < VGA_WIDTH; x++) {
                VGA_MEMORY[y * VGA_WIDTH + x] = vga_history[slot][x];
            }
        } else {
            int sy = v - vga_history_count;
            for (int x = 0; x < VGA_WIDTH; x++) {
                VGA_MEMORY[y * VGA_WIDTH + x] = vga_screen[sy][x];
            }
        }
    }

    // Indikator scroll di sudut kanan atas layar (Kuning di atas latar Biru)
    char badge[24];
    badge[0] = ' ';
    badge[1] = '[';
    badge[2] = (char)0x18; // Karakter panah atas VGA CP437
    badge[3] = ' ';
    badge[4] = 'S';
    badge[5] = 'C';
    badge[6] = 'R';
    badge[7] = 'O';
    badge[8] = 'L';
    badge[9] = 'L';
    badge[10] = ':';
    badge[11] = ' ';
    badge[12] = '+';
    int bi = 13;
    int tmp = vga_scroll_offset;
    if (tmp == 0) {
        badge[bi++] = '0';
    } else {
        char numbuf[8];
        int ni = 0;
        while (tmp > 0 && ni < 7) {
            numbuf[ni++] = (char)('0' + (tmp % 10));
            tmp /= 10;
        }
        while (ni > 0) {
            badge[bi++] = numbuf[--ni];
        }
    }
    badge[bi++] = ']';
    badge[bi++] = ' ';
    badge[bi] = '\0';

    int start_col = VGA_WIDTH - bi - 1;
    if (start_col < 0) start_col = 0;
    for (int i = 0; i < bi && (start_col + i) < VGA_WIDTH; i++) {
        VGA_MEMORY[0 * VGA_WIDTH + (start_col + i)] = (unsigned short)badge[i] | ((unsigned short)0x1E << 8);
    }

    // Sembunyikan kursor di luar layar selama membaca scrollback
    vga_update_cursor(25, 0);
}

// Scroll seluruh layar ke atas sebanyak 1 baris
static void vga_scroll(void) {
    // 1. Simpan baris teratas (row 0) ke riwayat
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_history[vga_history_head][x] = vga_screen[0][x];
    }
    vga_history_head = (vga_history_head + 1) % VGA_HISTORY_MAX;
    if (vga_history_count < VGA_HISTORY_MAX) {
        vga_history_count++;
    }

    // 2. Geser buffer layar aktif ke atas 1 baris
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_screen[y][x] = vga_screen[y + 1][x];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_screen[VGA_HEIGHT - 1][x] = (unsigned short)' ' | ((unsigned short)vga_color << 8);
    }
    vga_row = VGA_HEIGHT - 1;
    vga_column = 0;

    // 3. Perbarui tampilan VGA
    if (vga_scroll_offset == 0) {
        for (int y = 0; y < VGA_HEIGHT - 1; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
            }
        }
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (unsigned short)' ' | ((unsigned short)vga_color << 8);
        }
    } else {
        if (vga_scroll_offset < vga_history_count) {
            vga_scroll_offset++;
        }
        vga_render();
    }
}

// Geser tampilan ke atas sebanyak N baris
void vga_scroll_up(int lines) {
    if (vga_history_count == 0 || lines <= 0) {
        return;
    }
    vga_scroll_offset += lines;
    if (vga_scroll_offset > vga_history_count) {
        vga_scroll_offset = vga_history_count;
    }
    vga_render();
}

// Geser tampilan ke bawah sebanyak N baris
void vga_scroll_down(int lines) {
    if (vga_scroll_offset == 0 || lines <= 0) {
        return;
    }
    vga_scroll_offset -= lines;
    if (vga_scroll_offset < 0) {
        vga_scroll_offset = 0;
    }
    vga_render();
}

// Reset posisi tampilan kembali ke layar live terbawah
void vga_scroll_reset(void) {
    if (vga_scroll_offset != 0) {
        vga_scroll_offset = 0;
        vga_render();
    }
}

// Ambil offset scroll saat ini
int vga_get_scroll_offset(void) {
    return vga_scroll_offset;
}

// Membersihkan seluruh layar VGA
void vga_clear(void) {
    vga_enable_cursor();
    vga_scroll_offset = 0;

    // Simpan baris yang ada isinya ke riwayat sebelum layar dikosongkan
    for (int y = 0; y <= vga_row && y < VGA_HEIGHT; y++) {
        int has_content = 0;
        for (int x = 0; x < VGA_WIDTH; x++) {
            char ch = (char)(vga_screen[y][x] & 0xFF);
            if (ch != ' ' && ch != '\0') {
                has_content = 1;
                break;
            }
        }
        if (has_content) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga_history[vga_history_head][x] = vga_screen[y][x];
            }
            vga_history_head = (vga_history_head + 1) % VGA_HISTORY_MAX;
            if (vga_history_count < VGA_HISTORY_MAX) {
                vga_history_count++;
            }
        }
    }

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const int index = y * VGA_WIDTH + x;
            vga_screen[y][x] = (unsigned short)' ' | ((unsigned short)vga_color << 8);
            VGA_MEMORY[index] = vga_screen[y][x];
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
        if (vga_scroll_offset == 0) {
            vga_update_cursor(vga_row, vga_column);
        } else {
            vga_render();
        }
        return;
    }

    if (c == '\r') {
        vga_column = 0;
        if (vga_scroll_offset == 0) {
            vga_update_cursor(vga_row, vga_column);
        }
        return;
    }

    if (c == '\b') {
        if (vga_column > 0) {
            vga_column--;
            vga_screen[vga_row][vga_column] = (unsigned short)' ' | ((unsigned short)vga_color << 8);
            if (vga_scroll_offset == 0) {
                VGA_MEMORY[vga_row * VGA_WIDTH + vga_column] = vga_screen[vga_row][vga_column];
                vga_update_cursor(vga_row, vga_column);
            }
        }
        return;
    }

    vga_screen[vga_row][vga_column] = (unsigned short)c | ((unsigned short)vga_color << 8);
    if (vga_scroll_offset == 0) {
        VGA_MEMORY[vga_row * VGA_WIDTH + vga_column] = vga_screen[vga_row][vga_column];
    }

    if (++vga_column >= VGA_WIDTH) {
        vga_column = 0;
        if (++vga_row >= VGA_HEIGHT) {
            vga_scroll();
        }
    }
    if (vga_scroll_offset == 0) {
        vga_update_cursor(vga_row, vga_column);
    }
}

// Menampilkan string/teks ke layar VGA
void vga_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}
