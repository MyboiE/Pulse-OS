#ifndef MEMORY_H
#define MEMORY_H

#include "kernel.h"

void memory_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void memory_get_stats(size_t* used, size_t* available);
void memory_self_test(void);
void print_memory_stats(void);

#endif