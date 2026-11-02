#ifndef MEMORY_H
#define MEMORY_H

#include "kernel.h"

void memory_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void print_memory_stats(void);

#endif