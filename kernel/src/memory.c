#include "../include/kernel.h"
#include "../include/memory.h"

#define HEAP_SIZE (1024u * 1024u)
#define ALIGNMENT 8u

typedef struct block {
    size_t size;
    struct block* next;
    int free;
} block_t;

static union {
    unsigned long long align;
    unsigned char bytes[HEAP_SIZE];
} heap;
static block_t* first;

#define BLOCK_HEADER_SIZE ((sizeof(block_t) + ALIGNMENT - 1u) & ~(ALIGNMENT - 1u))

static void* block_data(block_t* block) {
    return (unsigned char*)block + BLOCK_HEADER_SIZE;
}

static size_t align_size(size_t size) {
    return (size + ALIGNMENT - 1u) & ~(ALIGNMENT - 1u);
}

static int heap_contains(const void* ptr) {
    const unsigned char* p = ptr;
    return p >= heap.bytes + sizeof(block_t) && p < heap.bytes + HEAP_SIZE;
}

void memory_init(void) {
    first = (block_t*)heap.bytes;
    first->size = HEAP_SIZE - BLOCK_HEADER_SIZE;
    first->next = NULL;
    first->free = 1;
}

void* kmalloc(size_t size) {
    if (size == 0 || size > HEAP_SIZE - BLOCK_HEADER_SIZE) return NULL;
    size = align_size(size);
    for (block_t* block = first; block; block = block->next) {
        if (!block->free || block->size < size) continue;
        if (block->size >= size + BLOCK_HEADER_SIZE + ALIGNMENT) {
            block_t* split = (block_t*)((unsigned char*)block_data(block) + size);
            split->size = block->size - size - BLOCK_HEADER_SIZE;
            split->next = block->next;
            split->free = 1;
            block->next = split;
            block->size = size;
        }
        block->free = 0;
        return block_data(block);
    }
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr || !heap_contains(ptr)) return;
    block_t* target = NULL;
    for (block_t* block = first; block; block = block->next) {
        if (block_data(block) == ptr) { target = block; break; }
    }
    if (!target || target->free) return;
    target->free = 1;
    for (block_t* block = first; block && block->next;) {
        if (block->free && block->next->free) {
            block->size += BLOCK_HEADER_SIZE + block->next->size;
            block->next = block->next->next;
        } else {
            block = block->next;
        }
    }
}

void memory_get_stats(size_t* used, size_t* available) {
    size_t in_use = 0, free_bytes = 0;
    for (block_t* block = first; block; block = block->next) {
        if (block->free) free_bytes += block->size;
        else in_use += block->size;
    }
    if (used) *used = in_use;
    if (available) *available = free_bytes;
}

static void test_assert(int ok, const char* message) {
    const char* result = ok ? "[PASS] " : "[FAIL] ";
    vga_print(result);
    vga_print(message);
    vga_print("\n");
    serial_print(result);
    serial_print(message);
    serial_print("\n");
}

void memory_self_test(void) {
    size_t before_used, before_free, used, available;
    memory_get_stats(&before_used, &before_free);
    void* a = kmalloc(128);
    void* b = kmalloc(256);
    memory_get_stats(&used, &available);
    test_assert(a && b && used >= before_used + 384 && available < before_free,
                "allocation updates used/free capacity");
    kfree((unsigned char*)a + 1);
    kfree(a);
    kfree(a);
    kfree(b);
    memory_get_stats(&used, &available);
    test_assert(used == before_used && available == before_free,
                "free restores heap capacity");
    void* full = kmalloc(HEAP_SIZE);
    test_assert(full == NULL, "out-of-memory returns NULL without kernel crash");
    memory_get_stats(&used, &available);
    test_assert(used == before_used && available == before_free,
                "failed allocation leaves heap unchanged");
}

static void print_kb(size_t bytes) {
    char digits[12];
    unsigned int n = 0;
    size_t kb = bytes / 1024u;
    if (!kb) { vga_print("0"); return; }
    while (kb) { digits[n++] = (char)('0' + kb % 10u); kb /= 10u; }
    while (n) vga_putchar(digits[--n]);
}

void print_memory_stats(void) {
    size_t used, available;
    memory_get_stats(&used, &available);
    vga_print("\n--- STATUS MEMORI DINAMIS ---\nTotal heap : 1024 KB\nTerpakai   : ");
    print_kb(used);
    vga_print(" KB\nTersedia   : ");
    print_kb(available);
    vga_print(" KB\n");
}
