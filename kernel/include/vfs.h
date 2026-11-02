#ifndef VFS_H
#define VFS_H

#include "kernel.h"

#define MAX_FILENAME 32
#define MAX_FILE_SIZE 1024

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    size_t size;
    int used;
} ramdisk_file_t;

void vfs_init(void);
int vfs_create(const char* name);
int vfs_read(const char* name, char* buffer);
int vfs_write(const char* name, const char* data);
int vfs_delete(const char* name);
void vfs_list(void);

#endif