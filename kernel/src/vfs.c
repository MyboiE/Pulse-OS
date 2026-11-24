#include "../include/vfs.h"
#include "../include/kernel.h"
#include "../include/protect.h"
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
  vfs_create("visitor-readme.txt");
  vfs_write("visitor-readme.txt", "Selamat datang di VFS Pulse-OS RAMDisk!");
}

static int name_is_visitor_file(const char *name) {
  static const char prefix[] = "visitor-";
  for (size_t i = 0; i < sizeof(prefix) - 1; i++) {
    if (name[i] != prefix[i]) return 0;
  }
  return 1;
}

static int vfs_name_allowed(const char *name) {
  return name_is_visitor_file(name)
      ? protect_resource_allowed(RESOURCE_VISITOR_ADMIN)
      : protect_resource_allowed(RESOURCE_EMERGENCY);
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
  if (!name || !name[0] || !vfs_name_allowed(name))
    return -1;
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
  if (!name || !data || !vfs_name_allowed(name)) return -1;
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

int vfs_read(const char *name, char *buffer, size_t buffer_size) {
  if (!name || !buffer || !vfs_name_allowed(name)) return -1;
  int idx = find_file(name);
  if (idx == -1 || !buffer || buffer_size <= files[idx].size)
    return -1;
  my_strncpy(buffer, files[idx].data, files[idx].size);
  buffer[files[idx].size] = '\0';
  return 0;
}

int vfs_delete(const char *name) {
  if (!name || !vfs_name_allowed(name)) return -1;
  int idx = find_file(name);
  if (idx == -1)
    return -1;
  files[idx].used = 0;
  files[idx].size = 0;
  files[idx].name[0] = '\0';
  return 0;
}

static void vfs_test_result(int ok, const char *message) {
  const char *result = ok ? "[PASS] " : "[FAIL] ";
  vga_print(result);
  vga_print(message);
  vga_print("\n");
  serial_print(result);
  serial_print(message);
  serial_print("\n");
}

void vfs_self_test(void) {
  char data[MAX_FILE_SIZE + 1];
  char long_name[MAX_FILENAME + 1];
  char names[MAX_FILES][MAX_FILENAME];
  int created[MAX_FILES] = {0};
  int count = 0;

  for (int i = 0; i < MAX_FILE_SIZE; i++) data[i] = 'x';
  data[MAX_FILE_SIZE] = '\0';
  for (int i = 0; i <= MAX_FILENAME; i++) long_name[i] = 'n';
  long_name[MAX_FILENAME] = '\0';

  int ok = vfs_create("vfstest-base") == 0;
  vfs_test_result(ok, "create succeeds");
  vfs_test_result(vfs_create("vfstest-base") != 0, "duplicate filename rejected");
  vfs_test_result(vfs_read("vfstest-missing", data, sizeof(data)) != 0,
                  "missing file read rejected");
  vfs_test_result(vfs_write("vfstest-missing", "x") != 0,
                  "missing file write rejected");
  vfs_test_result(vfs_delete("vfstest-missing") != 0,
                  "missing file delete rejected");
  vfs_test_result(vfs_create(long_name) != 0, "overlong filename rejected");
  vfs_test_result(vfs_write("vfstest-base", data) != 0,
                  "oversized file data rejected");
  data[MAX_FILE_SIZE - 1] = '\0';
  int exact_write = ok && vfs_write("vfstest-base", data) == 0;
  vfs_test_result(exact_write, "maximum supported file size accepted");
  vfs_test_result(exact_write && vfs_read("vfstest-base", data, MAX_FILE_SIZE - 1) != 0,
                  "undersized read buffer rejected");
  vfs_test_result(exact_write && vfs_read("vfstest-base", data, MAX_FILE_SIZE) == 0,
                  "exact-fit read buffer accepted");
  if (ok) vfs_delete("vfstest-base");

  for (int i = 0; i < MAX_FILES; i++) {
    names[i][0] = 'v'; names[i][1] = 'f'; names[i][2] = 's';
    names[i][3] = 't'; names[i][4] = '0';
    names[i][5] = (char)('0' + (i / 10));
    names[i][6] = (char)('0' + (i % 10)); names[i][7] = '\0';
    if (vfs_create(names[i]) == 0) {
      created[i] = 1;
      count++;
    } else {
      break;
    }
  }
  vfs_test_result(count > 0 && vfs_create("vfstest-overflow") != 0,
                  "full RAMDisk rejects create");
  for (int i = 0; i < MAX_FILES; i++)
    if (created[i]) vfs_delete(names[i]);
}

void vfs_list(void) {
  vga_print("\n--- DAFTAR FILE VFS (RAMDisk) ---\n");
  int count = 0;
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used && vfs_name_allowed(files[i].name)) {
      vga_print("- ");
      vga_print(files[i].name);
      vga_print("\n");
      serial_print("- ");
      serial_print(files[i].name);
      serial_print("\n");
      count++;
    }
  }
  if (count == 0)
    vga_print("(VFS Kosong)\n");
}

int test_input_overflow(void) {
  vfs_create("ovfl-chk.tmp");
  char big_buf[MAX_FILE_SIZE + 4];
  for (int i = 0; i < MAX_FILE_SIZE + 3; i++) big_buf[i] = 'X';
  big_buf[MAX_FILE_SIZE + 3] = '\0';
  int res = vfs_write("ovfl-chk.tmp", big_buf);
  vfs_delete("ovfl-chk.tmp");
  return (res != 0); /* Berhasil jika penulisan melebihi kapasitas ditolak */
}

int test_visitor_denial(void) {
  user_role_t old_role = protect_current_role();
  protect_set_role(ROLE_VISITOR_ADMIN);

  int denied_create = (vfs_create("patient-unauth.tmp") != 0);
  int denied_write  = (vfs_write("patient-A.txt", "unauth") != 0);
  int denied_read   = (vfs_read("patient-A.txt", (char[16]){0}, 16) != 0);
  int denied_delete = (vfs_delete("patient-A.txt") != 0);

  int allowed_visitor = (vfs_create("visitor-demo.tmp") == 0);
  vfs_delete("visitor-demo.tmp");

  protect_set_role(old_role);
  return (denied_create && denied_write && denied_read && denied_delete && allowed_visitor);
}

