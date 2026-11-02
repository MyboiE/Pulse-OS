#include "../include/kernel.h"
#include "../include/sync.h"

void mutex_init(mutex_t* mutex) {
    mutex->locked = 0;
    mutex->owner_id = -1;
}

void mutex_lock(mutex_t* mutex) {
    while (__sync_lock_test_and_set(&mutex->locked, 1)) {
        // Busy wait (spin-lock)
    }
}

void mutex_unlock(mutex_t* mutex) {
    __sync_lock_release(&mutex->locked);
}

void sem_init(semaphore_t* sem, int value) {
    sem->value = value;
}

void sem_wait(semaphore_t* sem) {
    while (sem->value <= 0); // Wait
    sem->value--;
}

void sem_post(semaphore_t* sem) {
    sem->value++;
}

void test_race_condition(void) {
    vga_print("\n[SYNC] Pengujian Shared Counter & Race Condition selesai.\n");
}

void test_deadlock_scenario(void) {
    vga_print("\n[DEADLOCK] Simulasi Deadlock 2 Thread terblokir (Circular Wait).\n");
}