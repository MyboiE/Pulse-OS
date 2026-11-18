#include "../include/kernel.h"
#include "../include/sync.h"
#include "../include/task.h"

/* ============================================================================
 * Helper Output Dual-Channel (VGA & Serial) - Dipanggil HANYA di luar lock
 * ============================================================================ */
static void sync_print(const char* str) {
    vga_print(str);
    serial_print(str);
}

static void sync_print_int(int val) {
    if (val < 0) {
        vga_putchar('-');
        serial_putchar('-');
        val = -val;
    }
    if (val >= 10) {
        sync_print_int(val / 10);
    }
    char c = (char)('0' + (val % 10));
    vga_putchar(c);
    serial_putchar(c);
}

/* ============================================================================
 * 1. IMPLEMENTASI MUTEX (Strict Ownership + Scheduler Block/Wake)
 * ============================================================================ */
void mutex_init(mutex_t* mutex) {
    if (!mutex) return;

    /* Invariant: Cegah double init jika mutex sedang terkunci atau punya waiter */
    if (mutex->initialized == 1 && (mutex->locked || mutex->wait_count > 0)) {
        sync_print("[SYNC WARN] mutex_init diabaikan: mutex masih aktif digunakan.\n");
        return;
    }

    mutex->locked = 0;
    mutex->owner_id = -1;
    mutex->wait_head = 0;
    mutex->wait_tail = 0;
    mutex->wait_count = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        mutex->wait_queue[i] = -1;
    }
    mutex->initialized = 1;
}

void mutex_lock(mutex_t* mutex) {
    if (!mutex || mutex->initialized != 1) {
        sync_print("[SYNC ERROR] mutex_lock gagal: mutex belum diinisialisasi.\n");
        return;
    }

    int tid = task_get_current_id();

    /* Invariant: Cegah self-deadlock (non-recursive mutex) */
    if (mutex->locked && mutex->owner_id == tid && tid != -1) {
        sync_print("[SYNC ERROR] Thread ");
        sync_print_int(tid);
        sync_print(" mencoba lock ulang mutex yang sudah dimilikinya.\n");
        return;
    }

    if (!mutex->locked) {
        /* Lock bebas: langsung ambil kepemilikan */
        mutex->locked = 1;
        mutex->owner_id = tid;
        return;
    }

    /* Lock sedang dimiliki thread lain: masukkan ke antrean FIFO & block thread */
    if (tid < 0 || mutex->wait_count >= MAX_THREADS) {
        sync_print("[SYNC ERROR] Antrean waiter mutex penuh atau ID thread tidak valid.\n");
        return;
    }

    mutex->wait_queue[mutex->wait_tail] = tid;
    mutex->wait_tail = (mutex->wait_tail + 1) % MAX_THREADS;
    mutex->wait_count++;

    /* Ubah status RUNNING -> WAITING lewat API resmi scheduler (tanpa busy-wait) */
    thread_block();

    /* Saat bangun dari thread_block(), ownership sudah diserahkan oleh unlock() */
}

void mutex_unlock(mutex_t* mutex) {
    if (!mutex || mutex->initialized != 1) {
        sync_print("[SYNC ERROR] mutex_unlock gagal: mutex belum diinisialisasi.\n");
        return;
    }

    if (!mutex->locked) {
        sync_print("[SYNC WARN] mutex_unlock diabaikan: mutex sudah dalam keadaan bebas.\n");
        return;
    }

    int tid = task_get_current_id();

    /* Invariant: Tolak unlock oleh thread yang bukan pemilik sah */
    if (mutex->owner_id != tid) {
        sync_print("[SYNC ERROR] Unlock ditolak! Pemilik sah ID=");
        sync_print_int(mutex->owner_id);
        sync_print(", pemanggil ID=");
        sync_print_int(tid);
        sync_print("\n");
        return;
    }

    /* Jika ada thread yang mengantre, serahkan ownership langsung (handoff) */
    if (mutex->wait_count > 0) {
        int next_tid = mutex->wait_queue[mutex->wait_head];
        mutex->wait_queue[mutex->wait_head] = -1;
        mutex->wait_head = (mutex->wait_head + 1) % MAX_THREADS;
        mutex->wait_count--;

        /* Tetapkan pemilik baru SEBELUM memanggil thread_wake (yang bisa preempt) */
        mutex->locked = 1;
        mutex->owner_id = next_tid;

        thread_wake(next_tid);
    } else {
        /* Tidak ada waiter: bebaskan lock */
        mutex->locked = 0;
        mutex->owner_id = -1;
    }
}

/* ============================================================================
 * 2. IMPLEMENTASI SEMAPHORE (Counting Semaphore + Scheduler Block/Wake)
 * ============================================================================ */
void sem_init(semaphore_t* sem, int value) {
    if (!sem) return;
    if (sem->initialized == 1 && sem->wait_count > 0) {
        sync_print("[SYNC WARN] sem_init diabaikan: masih ada waiter aktif.\n");
        return;
    }
    sem->value = value;
    sem->wait_head = 0;
    sem->wait_tail = 0;
    sem->wait_count = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        sem->wait_queue[i] = -1;
    }
    sem->initialized = 1;
}

void sem_wait(semaphore_t* sem) {
    if (!sem || sem->initialized != 1) return;

    if (sem->value > 0) {
        sem->value--;
        return;
    }

    int tid = task_get_current_id();
    if (tid < 0 || sem->wait_count >= MAX_THREADS) return;

    sem->wait_queue[sem->wait_tail] = tid;
    sem->wait_tail = (sem->wait_tail + 1) % MAX_THREADS;
    sem->wait_count++;

    thread_block();
}

void sem_post(semaphore_t* sem) {
    if (!sem || sem->initialized != 1) return;

    if (sem->wait_count > 0) {
        int next_tid = sem->wait_queue[sem->wait_head];
        sem->wait_queue[sem->wait_head] = -1;
        sem->wait_head = (sem->wait_head + 1) % MAX_THREADS;
        sem->wait_count--;

        thread_wake(next_tid);
    } else {
        sem->value++;
    }
}

/* ============================================================================
 * 3. SHARED RESOURCE SIMULASI IGD (Critical Section Singkat Tanpa Print)
 * ============================================================================ */
static mutex_t igd_room_mutex;
static int igd_available_rooms = 2;
static int igd_total_treated = 0;

void igd_sync_init(int initial_rooms) {
    igd_room_mutex.initialized = 0;
    mutex_init(&igd_room_mutex);
    igd_available_rooms = initial_rooms;
    igd_total_treated = 0;
}

int igd_use_shared_room(const char* patient_name) {
    int tid = task_get_current_id();
    int room_snapshot = 0;
    int treated_snapshot = 0;

    sync_print("  [IGD-LOCK] Thread ");
    sync_print_int(tid);
    sync_print(" (");
    sync_print(patient_name);
    sync_print(") meminta lock ruang tindakan...\n");

    mutex_lock(&igd_room_mutex);
    /* --- AWAL CRITICAL SECTION (Kecil & tanpa operasi print/yield) --- */
    if (igd_available_rooms > 0) {
        igd_available_rooms--;
    }
    igd_total_treated++;
    room_snapshot = igd_available_rooms;
    treated_snapshot = igd_total_treated;
    /* --- AKHIR CRITICAL SECTION --- */
    mutex_unlock(&igd_room_mutex);

    sync_print("  [IGD-LOCK] Thread ");
    sync_print_int(tid);
    sync_print(" (");
    sync_print(patient_name);
    sync_print(") masuk & melepas lock | Sisa ruang=");
    sync_print_int(room_snapshot);
    sync_print(", Total ditangani=");
    sync_print_int(treated_snapshot);
    sync_print("\n");

    return treated_snapshot;
}

/* ============================================================================
 * 4. UJI SHARED COUNTER: TANPA LOCK (RACE CONDITION) VS DENGAN LOCK
 * ============================================================================ */
#define COUNTER_OPS_PER_THREAD 50

static volatile int shared_counter = 0;
static mutex_t counter_mutex;

static void worker_nolock(void) {
    for (int i = 0; i < COUNTER_OPS_PER_THREAD; i++) {
        /* Operasi baca-ubah-tulis dipaksa interleave dengan thread_yield() */
        int temp = shared_counter;
        thread_yield();
        shared_counter = temp + 1;
    }
}

static void worker_withlock(void) {
    for (int i = 0; i < COUNTER_OPS_PER_THREAD; i++) {
        mutex_lock(&counter_mutex);
        /* Meskipun ada yield di dalam, mutex menjaga mutual exclusion */
        int temp = shared_counter;
        thread_yield();
        shared_counter = temp + 1;
        mutex_unlock(&counter_mutex);
    }
}

void test_race_condition(void) {
    int expected = COUNTER_OPS_PER_THREAD * 2;

    sync_print("\n===========================================================\n");
    sync_print("  UJI SHARED COUNTER: TANPA LOCK VS DENGAN MUTEX LOCK\n");
    sync_print("===========================================================\n");

    /* --- Skenario A: Kontrol Tanpa Lock --- */
    sync_print("\n[1] Menjalankan 2 thread TANPA lock (50 increment/thread)...\n");
    task_sys_init();
    shared_counter = 0;

    thread_create_prio("Worker-NoLock-1", worker_nolock, PRIO_SERIUS, 5, 1);
    thread_create_prio("Worker-NoLock-2", worker_nolock, PRIO_SERIUS, 5, 1);
    schedule();

    int result_nolock = shared_counter;
    sync_print("  >> Hasil TANPA lock  : ");
    sync_print_int(result_nolock);
    sync_print(" (Expected: ");
    sync_print_int(expected);
    sync_print(") -> ");
    if (result_nolock != expected) {
        sync_print("RACE CONDITION TERAMATI!\n");
    } else {
        sync_print("Kebetulan sama\n");
    }

    /* --- Skenario B: Dengan Mutex Lock --- */
    sync_print("\n[2] Menjalankan 2 thread DENGAN mutex lock (50 increment/thread)...\n");
    task_sys_init();
    shared_counter = 0;
    counter_mutex.initialized = 0;
    mutex_init(&counter_mutex);

    thread_create_prio("Worker-Lock-1", worker_withlock, PRIO_SERIUS, 5, 1);
    thread_create_prio("Worker-Lock-2", worker_withlock, PRIO_SERIUS, 5, 1);
    schedule();

    int result_withlock = shared_counter;
    sync_print("  >> Hasil DENGAN lock : ");
    sync_print_int(result_withlock);
    sync_print(" (Expected: ");
    sync_print_int(expected);
    sync_print(") -> ");
    if (result_withlock == expected) {
        sync_print("KONSISTEN & AMAN!\n");
    } else {
        sync_print("GAGAL!\n");
    }
    sync_print("===========================================================\n\n");
}

/* ============================================================================
 * 5. DEMONSTRASI DEADLOCK (CIRCULAR WAIT) & SOLUSI LOCK ORDERING
 * ============================================================================ */
static mutex_t lock_A; /* Resource ID 1: Ruang Tindakan */
static mutex_t lock_B; /* Resource ID 2: Alat Monitor */

/* Thread 1: Ambil Lock A lalu Lock B */
static void deadlock_thread_1(void) {
    sync_print("  [DL-T1] Meminta Lock A (Ruang Tindakan)...\n");
    mutex_lock(&lock_A);
    sync_print("  [DL-T1] Berhasil memegang Lock A. Yield ke thread lain...\n");

    thread_yield();

    sync_print("  [DL-T1] Meminta Lock B (Alat Monitor) sambil memegang Lock A...\n");
    mutex_lock(&lock_B);

    sync_print("  [DL-T1] Berhasil memegang Lock A & Lock B! Melepas keduanya.\n");
    mutex_unlock(&lock_B);
    mutex_unlock(&lock_A);
}

/* Thread 2 (Urutan Berlawanan -> Memicu Circular Wait): Ambil Lock B lalu Lock A */
static void deadlock_thread_2_bad(void) {
    sync_print("  [DL-T2] Meminta Lock B (Alat Monitor)...\n");
    mutex_lock(&lock_B);
    sync_print("  [DL-T2] Berhasil memegang Lock B. Yield ke thread lain...\n");

    thread_yield();

    sync_print("  [DL-T2] Meminta Lock A (Ruang Tindakan) sambil memegang Lock B...\n");
    mutex_lock(&lock_A);

    sync_print("  [DL-T2] Berhasil memegang Lock B & Lock A! Melepas keduanya.\n");
    mutex_unlock(&lock_A);
    mutex_unlock(&lock_B);
}

/* Thread 2 (Solusi Global Lock Ordering): Selalu ambil Lock A sebelum Lock B */
static void deadlock_thread_2_ordered(void) {
    sync_print("  [DL-T2-SAFE] Meminta Lock A dulu (sesuai Global Lock Ordering)...\n");
    mutex_lock(&lock_A);
    sync_print("  [DL-T2-SAFE] Berhasil memegang Lock A. Yield...\n");

    thread_yield();

    sync_print("  [DL-T2-SAFE] Meminta Lock B...\n");
    mutex_lock(&lock_B);

    sync_print("  [DL-T2-SAFE] Berhasil memegang Lock A & B! Melepas B lalu A.\n");
    mutex_unlock(&lock_B);
    mutex_unlock(&lock_A);
}

void test_deadlock_scenario(void) {
    sync_print("\n===========================================================\n");
    sync_print("  DEMONSTRASI DEADLOCK (CIRCULAR WAIT - UJI TERISOLASI)\n");
    sync_print("  T1: Lock A -> Lock B  |  T2: Lock B -> Lock A\n");
    sync_print("  PERINGATAN: Kernel akan macet (WAITING permanen) saat deadlock.\n");
    sync_print("  Gunakan Ctrl+A lalu X di terminal untuk keluar dari QEMU.\n");
    sync_print("===========================================================\n\n");

    task_sys_init();
    lock_A.initialized = 0;
    lock_B.initialized = 0;
    mutex_init(&lock_A);
    mutex_init(&lock_B);

    thread_create_prio("Deadlock-T1", deadlock_thread_1, PRIO_SERIUS, 4, 2);
    thread_create_prio("Deadlock-T2", deadlock_thread_2_bad, PRIO_SERIUS, 4, 2);

    /* Scheduler akan menjalankan T1 dan T2 hingga keduanya saling menunggu */
    schedule();
}

void test_deadlock_resolved(void) {
    sync_print("\n===========================================================\n");
    sync_print("  MITIGASI DEADLOCK: GLOBAL LOCK ORDERING (Lock A -> Lock B)\n");
    sync_print("===========================================================\n\n");

    task_sys_init();
    lock_A.initialized = 0;
    lock_B.initialized = 0;
    mutex_init(&lock_A);
    mutex_init(&lock_B);

    thread_create_prio("Safe-T1", deadlock_thread_1, PRIO_SERIUS, 4, 2);
    thread_create_prio("Safe-T2", deadlock_thread_2_ordered, PRIO_SERIUS, 4, 2);

    schedule();

    sync_print("\n  >> [HASIL] Kedua thread selesai tanpa deadlock (Circular Wait hilang)!\n");
    sync_print("===========================================================\n\n");
}