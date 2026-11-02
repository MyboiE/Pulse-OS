#include "../include/kernel.h"
#include "../include/task.h"

#define MAX_THREADS 8

static thread_t thread_pool[MAX_THREADS];
static int thread_count = 0;
static int current_thread = -1;

void task_sys_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_pool[i].id = -1;
        thread_pool[i].state = THREAD_TERMINATED;
    }
    thread_count = 0;
}

thread_t* thread_create(const char* name, void (*entry)(void)) {
    if (thread_count >= MAX_THREADS) return NULL;

    int slot = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].state == THREAD_TERMINATED) {
            slot = i;
            break;
        }
    }

    if (slot == -1) return NULL;

    thread_t* t = &thread_pool[slot];
    t->id = slot;
    t->entry = entry;
    t->state = THREAD_NEW;

    // Salin nama thread
    int i = 0;
    while (name[i] != '\0' && i < 31) {
        t->name[i] = name[i];
        i++;
    }
    t->name[i] = '\0';

    t->state = THREAD_READY; // Siap dieksekusi
    thread_count++;
    return t;
}

void schedule(void) {
    if (thread_count == 0) return;

    // Algoritma Round Robin Sederhana
    int next = (current_thread + 1) % MAX_THREADS;
    while (thread_pool[next].state != THREAD_READY && thread_pool[next].state != THREAD_RUNNING) {
        next = (next + 1) % MAX_THREADS;
    }

    if (current_thread != -1 && thread_pool[current_thread].state == THREAD_RUNNING) {
        thread_pool[current_thread].state = THREAD_READY;
    }

    current_thread = next;
    thread_pool[current_thread].state = THREAD_RUNNING;
}

void print_thread_states(void) {
    vga_print("\n--- STATUS THREAD KERNEL ---\n");
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].id != -1) {
            vga_print("Thread ID ");
            vga_putchar('0' + thread_pool[i].id);
            vga_print(" [");
            vga_print(thread_pool[i].name);
            vga_print("]: ");

            switch (thread_pool[i].state) {
                case THREAD_NEW:        vga_print("NEW\n"); break;
                case THREAD_READY:      vga_print("READY\n"); break;
                case THREAD_RUNNING:    vga_print("RUNNING\n"); break;
                case THREAD_WAITING:    vga_print("WAITING\n"); break;
                case THREAD_TERMINATED: vga_print("TERMINATED\n"); break;
            }
        }
    }
}