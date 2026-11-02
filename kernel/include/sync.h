#ifndef SYNC_H
#define SYNC_H

typedef struct {
    int locked;
    int owner_id;
} mutex_t;

typedef struct {
    int value;
} semaphore_t;

void mutex_init(mutex_t* mutex);
void mutex_lock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);

void sem_init(semaphore_t* sem, int value);
void sem_wait(semaphore_t* sem);
void sem_post(semaphore_t* sem);

// Benchmark & Skenario Pengujian
void test_race_condition(void);
void test_deadlock_scenario(void);

#endif