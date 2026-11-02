#include "../include/kernel.h"
#include "../include/memory.h"

#define HEAP_SIZE (1024 * 1024) // Heap size 1MB

static unsigned char heap_memory[HEAP_SIZE];
static size_t heap_offset = 0;

void memory_init(void) {
    heap_offset = 0;
}

void* kmalloc(size_t size) {
    if (heap_offset + size > HEAP_SIZE) {
        vga_print("\n[ERROR] Out of Memory! Kernel Aman (No Crash).\n");
        return NULL;
    }
    void* ptr = &heap_memory[heap_offset];
    heap_offset += size;
    return ptr;
}

void kfree(void* ptr) {
    // Bump allocator sederhana tidak melakukan de-alokasi granular per blok
    (void)ptr;
}

void print_memory_stats(void) {
    vga_print("\n--- STATUS MEMORI DINAMIS ---\n");
    vga_print("Total Heap : 1024 KB\n");
    vga_print("Terpakai   : ");
    // Cetak perkiraan sederhana
    if (heap_offset == 0) vga_print("0 KB\n");
    else vga_print(">0 KB\n");
}