#ifndef TASK_H
#define TASK_H

/* ============================================================================
   task.h - Pengelolaan Proses & Thread Kernel Pulse-OS
   ============================================================================

   Struktur Data:
   - thread_state_t : Enum status thread (NEW, READY, RUNNING, WAITING, TERMINATED)
   - thread_t       : Thread Control Block (TCB)

   Mekanisme:
   - Cooperative scheduling via context switch (stack switching)
   - Round Robin scheduler
   - Transisi status: NEW -> READY -> RUNNING <-> WAITING -> TERMINATED
   ============================================================================ */

#define MAX_THREADS     8
#define THREAD_STACK_SIZE 4096  /* 4 KB stack per thread */

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
    unsigned char    stack[THREAD_STACK_SIZE];      /* Stack privat thread            */
    int              wait_ticks;                   /* Sisa tick menunggu (WAITING)   */
    int              has_run;                      /* Apakah sudah pernah dieksekusi */
} thread_t;

/* ---- Fungsi Pengelolaan Thread ---- */

/* Inisialisasi subsistem task */
void task_sys_init(void);

/* Membuat thread baru dengan nama dan fungsi entry point.
   Transisi status: -> NEW -> READY */
thread_t* thread_create(const char* name, void (*entry)(void));

/* Thread secara kooperatif menyerahkan CPU ke thread lain.
   Transisi status: RUNNING -> READY, lalu READY -> RUNNING (thread berikutnya) */
void thread_yield(void);

/* Thread memasuki status WAITING selama sejumlah tick.
   Transisi status: RUNNING -> WAITING */
void thread_wait(int ticks);

/* Menandai thread saat ini sebagai TERMINATED.
   Transisi status: RUNNING -> TERMINATED */
void thread_exit(void);

/* Menjalankan scheduler Round Robin (dipanggil oleh yield/wait/exit) */
void schedule(void);

/* Cetak status semua thread yang terdaftar */
void print_thread_states(void);

/* ---- Demo & Pengujian ---- */

/* Menjalankan demo 2 kernel thread dengan tugas berbeda,
   menunjukkan transisi semua 5 status */
void task_demo_run(void);

/* Mendapatkan jumlah thread aktif (non-TERMINATED) */
int task_get_active_count(void);

/* Mendapatkan pointer ke thread berdasarkan ID */
thread_t* task_get_thread(int id);

/* Mendapatkan ID thread yang sedang berjalan */
int task_get_current_id(void);

#endif
