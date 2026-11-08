#include "../include/vfs.h"
#include "../include/kernel.h"
// removed std lib include

static size_t my_strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0')
    len++;
  return len;
}

static int my_strcmp(const char *a, const char *b) {
  while (*a && (*a == *b)) {
    a++;
    b++;
  }
  return *(unsigned char *)a - *(unsigned char *)b;
}

static void my_strncpy(char *dest, const char *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i] != '\0'; i++)
    dest[i] = src[i];
  for (; i < n; i++)
    dest[i] = '\0';
}

#define MAX_FILES 16

static ramdisk_file_t files[MAX_FILES];

void vfs_init(void) {
  for (int i = 0; i < MAX_FILES; i++) {
    files[i].used = 0;
    files[i].size = 0;
  }
  // create default file
  vfs_create("readme.txt");
  vfs_write("readme.txt", "Selamat datang di VFS Pulse-OS RAMDisk!");
}

static int find_file(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used && my_strcmp(files[i].name, name) == 0) {
      return i;
    }
  }
  return -1;
}

int vfs_create(const char *name) {
  if (my_strlen(name) >= MAX_FILENAME)
    return -1; // name too long
  if (find_file(name) != -1)
    return -1; // already exists
  for (int i = 0; i < MAX_FILES; i++) {
    if (!files[i].used) {
      my_strncpy(files[i].name, name, MAX_FILENAME - 1);
      files[i].name[MAX_FILENAME - 1] = '\0';
      files[i].used = 1;
      files[i].size = 0;
      return 0;
    }
  }
  return -1; // VFS full
}

int vfs_write(const char *name, const char *data) {
  int idx = find_file(name);
  if (idx == -1)
    return -1; // not found
  size_t len = my_strlen(data);
  if (len >= MAX_FILE_SIZE)
    return -1; // data too large
  my_strncpy(files[idx].data, data, MAX_FILE_SIZE - 1);
  files[idx].data[MAX_FILE_SIZE - 1] = '\0';
  files[idx].size = len;
  return 0;
}

int vfs_read(const char *name, char *buffer) {
  int idx = find_file(name);
  if (idx == -1)
    return -1;
  my_strncpy(buffer, files[idx].data, files[idx].size);
  buffer[files[idx].size] = '\0';
  return 0;
}

int vfs_delete(const char *name) {
  int idx = find_file(name);
  if (idx == -1)
    return -1;
  files[idx].used = 0;
  files[idx].size = 0;
  files[idx].name[0] = '\0';
  return 0;
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
  if (count == 0)
    vga_print("(VFS Kosong)\n");
}
