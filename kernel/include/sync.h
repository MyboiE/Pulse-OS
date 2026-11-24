#ifndef SYNC_H
#define SYNC_H

#include "task.h" /* [TAMBAHAN] Akses konstanta MAX_THREADS */

/* ============================================================================
 * [TAMBAHAN] DOKUMENTASI KONTRAK API SINKRONISASI
 * - Primitive : Mutex (strict ownership) & Semaphore.
 * - Perilaku  : Jika terkunci, thread masuk wait_queue dan memanggil
 *               thread_block() (WAITING), bukan busy-wait.
 * - Ownership : Hanya thread dengan id == owner_id yang boleh mutex_unlock().
 * - Interrupt : TIDAK AMAN dipanggil dari interrupt handler (ISR) karena
 *               dapat memicu blocking dan context switch scheduler.
 * ============================================================================ */

typedef struct {
    int locked;                     /* Kode asli dipertahankan */
    int owner_id;                   /* Kode asli dipertahankan */
    int initialized;                /* [TAMBAHAN] Cegah double init merusak state */
    int wait_queue[MAX_THREADS];    /* [TAMBAHAN] Antrean FIFO thread terblokir */
    int wait_head;                  /* [TAMBAHAN] Indeks keluar antrean */
    int wait_tail;                  /* [TAMBAHAN] Indeks masuk antrean */
    int wait_count;                 /* [TAMBAHAN] Jumlah thread yang menunggu */
} mutex_t;

typedef struct {
    int value;                      /* Kode asli dipertahankan */
    int initialized;                /* [TAMBAHAN] Cegah double init */
    int wait_queue[MAX_THREADS];    /* [TAMBAHAN] Antrean FIFO waiter */
    int wait_head;                  /* [TAMBAHAN] Indeks keluar antrean */
    int wait_tail;                  /* [TAMBAHAN] Indeks masuk antrean */
    int wait_count;                 /* [TAMBAHAN] Jumlah waiter */
} semaphore_t;

/* API Asli Dipertahankan Sepenuhnya */
void mutex_init(mutex_t* mutex);
void mutex_lock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);

void sem_init(semaphore_t* sem, int value);
void sem_wait(semaphore_t* sem);
void sem_post(semaphore_t* sem);

// Benchmark & Skenario Pengujian (Asli Dipertahankan)
void test_race_condition(void);
void test_deadlock_scenario(void);

/* [TAMBAHAN] API Resource IGD & Mitigasi Deadlock (Lock Ordering) */
void igd_sync_init(int initial_rooms);
int  igd_use_shared_room(const char* patient_name);
void test_deadlock_resolved(void);
int  test_lock_handoff(void);

#endif