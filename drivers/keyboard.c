#include "../kernel/include/kernel.h"

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64

static int shift_state = 0;
static int caps_state = 0;

// Mapping Scancode Set 1 (US QWERTY) tanpa Shift
static const char scancode_ascii[128] = {
    [0x01] = 27,   [0x02] = '1',  [0x03] = '2',  [0x04] = '3',  [0x05] = '4',
    [0x06] = '5',  [0x07] = '6',  [0x08] = '7',  [0x09] = '8',  [0x0A] = '9',
    [0x0B] = '0',  [0x0C] = '-',  [0x0D] = '=',  [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q',  [0x11] = 'w',  [0x12] = 'e',  [0x13] = 'r',  [0x14] = 't',
    [0x15] = 'y',  [0x16] = 'u',  [0x17] = 'i',  [0x18] = 'o',  [0x19] = 'p',
    [0x1A] = '[',  [0x1B] = ']',  [0x1C] = '\n',
    [0x1E] = 'a',  [0x1F] = 's',  [0x20] = 'd',  [0x21] = 'f',  [0x22] = 'g',
    [0x23] = 'h',  [0x24] = 'j',  [0x25] = 'k',  [0x26] = 'l',  [0x27] = ';',
    [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\', [0x2C] = 'z',  [0x2D] = 'x',  [0x2E] = 'c',  [0x2F] = 'v',
    [0x30] = 'b',  [0x31] = 'n',  [0x32] = 'm',  [0x33] = ',',  [0x34] = '.',
    [0x35] = '/',  [0x39] = ' ',
};

// Mapping Scancode Set 1 (US QWERTY) dengan Shift
static const char scancode_ascii_shift[128] = {
    [0x01] = 27,   [0x02] = '!',  [0x03] = '@',  [0x04] = '#',  [0x05] = '$',
    [0x06] = '%',  [0x07] = '^',  [0x08] = '&',  [0x09] = '*',  [0x0A] = '(',
    [0x0B] = ')',  [0x0C] = '_',  [0x0D] = '+',  [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'Q',  [0x11] = 'W',  [0x12] = 'E',  [0x13] = 'R',  [0x14] = 'T',
    [0x15] = 'Y',  [0x16] = 'U',  [0x17] = 'I',  [0x18] = 'O',  [0x19] = 'P',
    [0x1A] = '{',  [0x1B] = '}',  [0x1C] = '\n',
    [0x1E] = 'A',  [0x1F] = 'S',  [0x20] = 'D',  [0x21] = 'F',  [0x22] = 'G',
    [0x23] = 'H',  [0x24] = 'J',  [0x25] = 'K',  [0x26] = 'L',  [0x27] = ':',
    [0x28] = '"',  [0x29] = '~',
    [0x2B] = '|',  [0x2C] = 'Z',  [0x2D] = 'X',  [0x2E] = 'C',  [0x2F] = 'V',
    [0x30] = 'B',  [0x31] = 'N',  [0x32] = 'M',  [0x33] = '<',  [0x34] = '>',
    [0x35] = '?',  [0x39] = ' ',
};

// Inisialisasi controller keyboard PS/2
void keyboard_init(void) {
    shift_state = 0;
    caps_state = 0;
    // Bersihkan buffer keyboard jika ada sisa data saat boot
    while (inb(KBD_STATUS_PORT) & 1) {
        inb(KBD_DATA_PORT);
    }
}

// Cek non-blocking apakah ada karakter yang ditekan
char keyboard_poll(void) {
    if ((inb(KBD_STATUS_PORT) & 1) == 0) {
        return 0;
    }

    unsigned char scancode = inb(KBD_DATA_PORT);

    // Abaikan prefix tombol extended (0xE0)
    if (scancode == 0xE0) {
        return 0;
    }

    // Tombol dilepas (Release / Break Code: bit 7 aktif)
    if (scancode & 0x80) {
        unsigned char rel = scancode & 0x7F;
        if (rel == 0x2A) shift_state &= ~1; // Left Shift dilepas
        else if (rel == 0x36) shift_state &= ~2; // Right Shift dilepas
        return 0;
    }

    // Tombol ditekan (Make Code)
    if (scancode == 0x2A) {
        shift_state |= 1; // Left Shift ditekan
        return 0;
    }
    if (scancode == 0x36) {
        shift_state |= 2; // Right Shift ditekan
        return 0;
    }
    if (scancode == 0x3A) {
        caps_state ^= 1;  // Toggle Caps Lock
        return 0;
    }

    if (scancode >= 128) {
        return 0;
    }

    char base = scancode_ascii[scancode];
    if (base == 0) {
        return 0;
    }

    int is_letter = (base >= 'a' && base <= 'z');
    int use_shift = is_letter ? ((shift_state != 0) ^ caps_state) : (shift_state != 0);

    return use_shift ? scancode_ascii_shift[scancode] : base;
}

// Membaca karakter dari keyboard secara blocking
char keyboard_getchar(void) {
    char c = 0;
    while ((c = keyboard_poll()) == 0);
    return c;
}

// Input terpadu: membaca dari keyboard PS/2 (jendela QEMU) ATAU serial (terminal)
static int serial_last_cr = 0;

char input_getchar(void) {
    while (1) {
        char c = keyboard_poll();
        if (c != 0) {
            serial_last_cr = 0;
            return c;
        }
        if (serial_has_char()) {
            char sc = serial_getchar();
            if (serial_last_cr && sc == '\n') {
                serial_last_cr = 0;
                continue; // Abaikan LF tepat setelah CR dari terminal serial
            }
            if (sc == '\r') {
                serial_last_cr = 1;
                return '\n';
            }
            serial_last_cr = 0;
            return sc;
        }
    }
}
