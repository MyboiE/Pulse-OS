#include "../include/kernel.h"
#include "../include/vfs.h"

#define MAX_FILES 16

static ramdisk_file_t files[MAX_FILES];

void vfs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].size = 0;
    }
    // Buat file bawaan sistem
    vfs_create("readme.txt");
    vfs_write("readme.txt", "Selamat datang di VFS Pulse-OS RAMDisk!");
}

int vfs_create(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            int j = 0;
            while (name[j] != '\0' && j < MAX_FILENAME - 1) {
                files[i].name[j] = name[j];
                j++;
            }
            files[i].name[j] = '\0';
            files[i].used = 1;
            files[i].size = 0;
            return 0;
        }
    }
    return -1; // VFS Penuh
}

int vfs_write(const char* name, const char* data) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            // Simple string match
            int match = 1;
            for (int k = 0; name[k] != '\0'; k++) {
                if (files[i].name[k] != name[k]) { match = 0; break; }
            }
            if (match) {
                int j = 0;
                while (data[j] != '\0' && j < MAX_FILE_SIZE - 1) {
                    files[i].data[j] = data[j];
                    j++;
                }
                files[i].data[j] = '\0';
                files[i].size = j;
                return 0;
            }
        }
    }
    return -1; // File tidak ditemukan
}

int vfs_read(const char* name, char* buffer) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            int match = 1;
            for (int k = 0; name[k] != '\0'; k++) {
                if (files[i].name[k] != name[k]) { match = 0; break; }
            }
            if (match) {
                for (size_t j = 0; j < files[i].size; j++) {
                    buffer[j] = files[i].data[j];
                }
                buffer[files[i].size] = '\0';
                return 0;
            }
        }
    }
    return -1;
}

int vfs_delete(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            int match = 1;
            for (int k = 0; name[k] != '\0'; k++) {
                if (files[i].name[k] != name[k]) { match = 0; break; }
            }
            if (match) {
                files[i].used = 0;
                return 0;
            }
        }
    }
    return -1;
}

void vfs_list(void) {
    vga_print("\n--- DAFTAR FILE VFS (RAMDisk) ---\n");
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            vga_print("- ");
            vga_print(files[i].name);
            vga_print("\n");
            count++;
        }
    }
    if (count == 0) vga_print("(VFS Kosong)\n");
}