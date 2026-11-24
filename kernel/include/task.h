#ifndef TASK_H
#define TASK_H

#define MAX_THREADS       8
#define THREAD_STACK_SIZE 4096  /* 4 KB stack per thread */

/* Kelas Triase / Prioritas Utama (angka lebih kecil = prioritas lebih tinggi) */
#define PRIO_KRITIS       0
#define PRIO_SERIUS       1
#define PRIO_STABIL       2
#define PRIO_DEFAULT      1

/* Status thread sesuai teori Sistem Operasi */
typedef enum {
    THREAD_NEW,         /* Thread baru dibuat, belum siap dijadwalkan       */
    THREAD_READY,       /* Thread siap dieksekusi, menunggu giliran CPU     */
    THREAD_RUNNING,     /* Thread sedang aktif dieksekusi oleh CPU          */
    THREAD_WAITING,     /* Thread menunggu suatu event/kondisi              */
    THREAD_TERMINATED   /* Thread selesai atau dihentikan                   */
} thread_state_t;

/* Thread Control Block (TCB) - Struktur data inti pengelolaan thread */
typedef struct thread {
    int              id;                           /* ID unik thread                 */
    char             name[32];                     /* Nama thread (deskriptif)       */
    thread_state_t   state;                        /* Status thread saat ini         */
    void           (*entry)(void);                 /* Fungsi entry point thread      */
    unsigned int     esp;                          /* Saved stack pointer (ESP)      */
    unsigned char    stack[THREAD_STACK_SIZE];     /* Stack privat thread            */
    int              wait_ticks;                   /* Sisa tick menunggu (WAITING)   */
    int              has_run;                      /* Apakah sudah pernah dieksekusi */

    /* Atribut Preemptive Priority Queue Scheduler */
    int              triage_class;                 /* 0: Kritis, 1: Serius, 2: Stabil */
    int              remaining_work;               /* Estimasi sisa unit kerja        */
    int              resource_load;                /* Beban resource terukur          */
    int              hybrid_score;                 /* Skor hybrid intra-kelas (0-100) */
    unsigned int     enqueue_order;                /* Urutan masuk READY (tie-break)  */
} thread_t;

/* ---- Fungsi Pengelolaan Thread ---- */
void task_sys_init(void);
thread_t* thread_create(const char* name, void (*entry)(void));
thread_t* thread_create_prio(const char* name, void (*entry)(void),
                             int triage_class, int remaining_work, int resource_load);
void thread_yield(void);
void thread_wait(int ticks);
void thread_exit(void);
void schedule(void);
void scheduler_tick(void);
void thread_block(void);
void thread_wake(int thread_id);
void thread_update_work(int remaining_work);
void print_thread_states(void);

/* ---- Demo & Pengujian ---- */
void task_demo_run(void);
void task_run_tests(void);
void test_hybrid_fifo(void);
void test_thread_pool_exhaustion(void);
void test_preemptive_timer(void);
int task_get_active_count(void);
thread_t* task_get_thread(int id);
int task_get_current_id(void);

#endif
