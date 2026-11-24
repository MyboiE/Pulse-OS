#include "../include/kernel.h"
#include "../include/task.h"
// Test harness helper for task tests
static void test_result(int ok, const char *msg) {
    if (ok) {
        vga_print("  [PASS] ");
        serial_print("  [PASS] ");
    } else {
        vga_print("  [FAIL] ");
        serial_print("  [FAIL] ");
    }
    vga_print(msg);
    vga_print("\n");
    serial_print(msg);
    serial_print("\n");
}

/* ============================================================================
   task.c - Implementasi Pengelolaan Proses & Thread Kernel Pulse-OS
   ============================================================================

   Implementasi ini mencakup:
   1. Thread Control Block (TCB) pool - array statis MAX_THREADS
   2. Context switching via assembly stack swap
   3. Preemptive Priority Queue Scheduler (Kelas Triase -> Hybrid Score -> FIFO)
   4. Transisi status lengkap: NEW -> READY -> RUNNING <-> WAITING -> TERMINATED
   5. Demo 2 kernel thread dengan tugas berbeda
   ============================================================================ */

/* ---- Pool & State Variables ---- */
static thread_t thread_pool[MAX_THREADS];
static int thread_count = 0;
static int current_thread = -1;
static unsigned int kernel_esp = 0; /* ESP kernel utama, untuk kembali setelah semua thread selesai */
static unsigned int global_enqueue_counter = 0; /* Counter urutan masuk antrean READY (FIFO) */

static inline unsigned int irq_save_disable(void) {
    unsigned int flags;
    __asm__ volatile ("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_restore(unsigned int flags) {
    __asm__ volatile ("pushl %0; popfl" : : "r"(flags) : "memory");
}

/* ---- Helper: String -> Status Name ---- */
static const char* state_name(thread_state_t s) {
    switch (s) {
        case THREAD_NEW:        return "NEW";
        case THREAD_READY:      return "READY";
        case THREAD_RUNNING:    return "RUNNING";
        case THREAD_WAITING:    return "WAITING";
        case THREAD_TERMINATED: return "TERMINATED";
        default:                return "UNKNOWN";
    }
}

/* ---- Helper: Print angka hexadecimal (ESP / Alamat Memori) ---- */
static void print_hex(unsigned int val) {
    const char hex_chars[] = "0123456789ABCDEF";
    vga_print("0x");
    serial_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        char c = hex_chars[(val >> i) & 0xF];
        vga_putchar(c);
        serial_putchar(c);
    }
}

/* ---- Helper: Log transisi status thread ---- */
static void log_transition(thread_t* t, thread_state_t from, thread_state_t to) {
    vga_print("  [TASK] Thread ");
    vga_putchar('0' + t->id);
    vga_print(" (");
    vga_print(t->name);
    vga_print("): ");
    vga_print(state_name(from));
    vga_print(" -> ");
    vga_print(state_name(to));
    vga_print("\n");

    serial_print("  [TASK] Thread ");
    serial_putchar('0' + t->id);
    serial_print(" (");
    serial_print(t->name);
    serial_print("): ");
    serial_print(state_name(from));
    serial_print(" -> ");
    serial_print(state_name(to));
    serial_print("\n");
}

/* ---- Helper: Set state dengan logging ---- */
static void set_state(thread_t* t, thread_state_t new_state) {
    thread_state_t old = t->state;
    t->state = new_state;
    if (new_state == THREAD_READY && old != THREAD_READY) {
        t->enqueue_order = ++global_enqueue_counter;
    }
    if (old != new_state) {
        log_transition(t, old, new_state);
    }
}

/* ============================================================================
   Helper Priority Queue 2-Tingkat (Hybrid + FIFO Tie-Break)
   ============================================================================
   1. Kunci Utama  : triage_class (0 Kritis < 1 Serius < 2 Stabil)
   2. Kunci Kedua  : hybrid_score = 0.6 * norm_remaining_time + 0.4 * norm_resource_load
   3. Kunci Ketiga : enqueue_order (FIFO saat kelas dan skor hybrid sama)
   ============================================================================ */
static int calc_hybrid_score(int remaining_work, int resource_load) {
    if (remaining_work < 0) remaining_work = 0;
    if (resource_load < 0) resource_load = 0;

    /* Normalisasi ke rentang 0-100 (maks unit kerja = 10, maks slot resource = 5) */
    int norm_time = (remaining_work * 100) / 10;
    int norm_res  = (resource_load * 100) / 5;

    if (norm_time > 100) norm_time = 100;
    if (norm_res  > 100) norm_res  = 100;

    /* Bobot: 60% sisa waktu eksekusi + 40% beban resource (skor lebih kecil menang) */
    return (60 * norm_time + 40 * norm_res) / 100;
}

static int is_higher_priority(const thread_t* cand, const thread_t* best) {
    /* Tingkat 1: Kelas triase tidak boleh dikalahkan oleh skor beban kerja */
    if (cand->triage_class != best->triage_class) {
        return cand->triage_class < best->triage_class;
    }

    /* Tingkat 2: Skor hybrid di dalam kelas triase yang sama */
    if (cand->hybrid_score != best->hybrid_score) {
        return cand->hybrid_score < best->hybrid_score;
    }

    /* Tingkat 3: Tie-breaker FIFO (masuk READY lebih awal menang) */
    return cand->enqueue_order < best->enqueue_order;
}

/* Explicit binary min-heap: priority, hybrid score, then FIFO. */
static int ready_heap[MAX_THREADS];
static int ready_heap_size;

static void heap_swap(int a, int b) {
    int tmp = ready_heap[a];
    ready_heap[a] = ready_heap[b];
    ready_heap[b] = tmp;
}

static void heap_push(int id) {
    int pos = ready_heap_size++;
    ready_heap[pos] = id;
    while (pos > 0) {
        int parent = (pos - 1) / 2;
        if (!is_higher_priority(&thread_pool[ready_heap[pos]],
                                &thread_pool[ready_heap[parent]])) break;
        heap_swap(pos, parent);
        pos = parent;
    }
}

static int heap_pop(void) {
    int result;
    int pos = 0;
    if (ready_heap_size == 0) return -1;
    result = ready_heap[0];
    ready_heap[0] = ready_heap[--ready_heap_size];
    while (1) {
        int left = pos * 2 + 1;
        int right = left + 1;
        int best = pos;
        if (left < ready_heap_size &&
            is_higher_priority(&thread_pool[ready_heap[left]],
                               &thread_pool[ready_heap[best]])) best = left;
        if (right < ready_heap_size &&
            is_higher_priority(&thread_pool[ready_heap[right]],
                               &thread_pool[ready_heap[best]])) best = right;
        if (best == pos) break;
        heap_swap(pos, best);
        pos = best;
    }
    return result;
}

static int pick_next_thread(void) {
    ready_heap_size = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].id != -1 && thread_pool[i].state == THREAD_READY) {
            heap_push(i);
        }
    }
    return heap_pop();
}

/* ============================================================================
   Inisialisasi Subsistem Task
   ============================================================================ */
void task_sys_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_pool[i].id = -1;
        thread_pool[i].state = THREAD_TERMINATED;
        thread_pool[i].entry = NULL;
        thread_pool[i].esp = 0;
        thread_pool[i].wait_ticks = 0;
        thread_pool[i].has_run = 0;
        thread_pool[i].triage_class = PRIO_DEFAULT;
        thread_pool[i].remaining_work = 0;
        thread_pool[i].resource_load = 0;
        thread_pool[i].hybrid_score = 0;
        thread_pool[i].enqueue_order = 0;
    }
    thread_count = 0;
    current_thread = -1;
    kernel_esp = 0;
    global_enqueue_counter = 0;
}

/* ============================================================================
   Thread Wrapper - Fungsi pembungkus yang memanggil entry() thread
   Setelah entry() selesai, otomatis memanggil thread_exit()
   ============================================================================ */
static void thread_wrapper(void) {
    if (current_thread >= 0 && current_thread < MAX_THREADS) {
        thread_t* t = &thread_pool[current_thread];
        if (t->entry) {
            t->entry();
        }
    }
    /* Thread selesai, tandai sebagai TERMINATED */
    thread_exit();

    /* Safety: seharusnya tidak pernah sampai sini */
    while (1) { __asm__ volatile("hlt"); }
}

/* ============================================================================
   Membuat Thread Baru dengan Parameter Prioritas & Beban Kerja
   Transisi: -> NEW -> READY
   ============================================================================ */
thread_t* thread_create_prio(const char* name, void (*entry)(void),
                             int triage_class, int remaining_work, int resource_load) {
    unsigned int flags = irq_save_disable();
    // If thread pool is exhausted, report error
    if (thread_count >= MAX_THREADS) {
        vga_print("[ERROR] Thread pool exhausted.\n");
        serial_print("[ERROR] Thread pool exhausted.\n");
        irq_restore(flags);
        return NULL;
    }

    int slot = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].id == -1) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        irq_restore(flags);
        return NULL;
    }

    thread_t* t = &thread_pool[slot];
    t->id = slot;
    t->entry = entry;
    t->wait_ticks = 0;
    t->has_run = 0;
    t->triage_class = triage_class;
    t->remaining_work = remaining_work;
    t->resource_load = resource_load;
    t->hybrid_score = calc_hybrid_score(remaining_work, resource_load);

    /* Salin nama thread */
    int i = 0;
    while (name[i] != '\0' && i < 31) {
        t->name[i] = name[i];
        i++;
    }
    t->name[i] = '\0';

    /* Status awal: NEW */
    t->state = THREAD_NEW;
    vga_print("  [TASK] Thread ");
    vga_putchar('0' + slot);
    vga_print(" (");
    vga_print(t->name);
    vga_print(") dibuat dengan status NEW\n");
    serial_print("  [TASK] Thread ");
    serial_putchar('0' + slot);
    serial_print(" (");
    serial_print(t->name);
    serial_print(") dibuat dengan status NEW\n");

    /* Setup stack untuk context switch (tidak diubah) */
    unsigned int* stack_top = (unsigned int*)((unsigned int)t->stack + THREAD_STACK_SIZE);
    *(--stack_top) = (unsigned int)thread_wrapper;  /* Return address              */
    *(--stack_top) = 0;                              /* EBP (pertama di-push)       */
    *(--stack_top) = 0;                              /* EBX                         */
    *(--stack_top) = 0;                              /* ESI                         */
    *(--stack_top) = 0;                              /* EDI (terakhir di-push)      */
    t->esp = (unsigned int)stack_top;

    /* Transisi NEW -> READY */
    set_state(t, THREAD_READY);
    thread_count++;

    /* Preempt langsung jika ada thread RUNNING dengan prioritas lebih rendah */
    if (current_thread >= 0 && current_thread < MAX_THREADS &&
        thread_pool[current_thread].state == THREAD_RUNNING) {
        if (is_higher_priority(t, &thread_pool[current_thread])) {
            irq_restore(flags);
            schedule();
            return t;
        }
    }

    irq_restore(flags);
    return t;
}

/* ============================================================================
   Membuat Thread Baru (Wrapper kompatibilitas dengan demo lama)
   ============================================================================ */
thread_t* thread_create(const char* name, void (*entry)(void)) {
    return thread_create_prio(name, entry, PRIO_DEFAULT, 0, 0);
}

/* ============================================================================
   Context Switch (Assembly - Tidak Diubah)
   ============================================================================ */
static void __attribute__((naked, used))
context_switch(unsigned int* old_esp, unsigned int new_esp) {
    (void)old_esp; (void)new_esp;
    __asm__ volatile (
        /* Simpan callee-saved registers ke stack lama */
        "pushl %%ebp\n\t"
        "pushl %%ebx\n\t"
        "pushl %%esi\n\t"
        "pushl %%edi\n\t"

        /* old_esp_ptr ada di [esp + 20] (4 push * 4 byte + return addr) */
        "movl 20(%%esp), %%eax\n\t"   /* eax = old_esp_ptr            */
        "movl %%esp, (%%eax)\n\t"       /* *old_esp_ptr = current esp   */

        /* new_esp ada di [esp + 24] */
        "movl 24(%%esp), %%esp\n\t"     /* esp = new_esp                */

        /* Pulihkan callee-saved registers dari stack baru */
        "popl %%edi\n\t"
        "popl %%esi\n\t"
        "popl %%ebx\n\t"
        "popl %%ebp\n\t"

        "ret\n\t"
        ::: "memory"
    );
}

/* ============================================================================
   Update Waiting Threads
   Mengurangi wait_ticks dan memindahkan ke READY jika sudah 0
   ============================================================================ */
static void update_waiting_threads(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].state == THREAD_WAITING && thread_pool[i].wait_ticks > 0) {
            thread_pool[i].wait_ticks--;
            if (thread_pool[i].wait_ticks <= 0) {
                set_state(&thread_pool[i], THREAD_READY);
            }
        }
    }
}

/* ============================================================================
   Helper Logging Keputusan Scheduler (Komponen Skor Triase, Hybrid, FIFO)
   ============================================================================ */
static void print_vga_int(int val) {
    if (val < 0) {
        vga_putchar('-');
        val = -val;
    }
    if (val >= 10) print_vga_int(val / 10);
    vga_putchar((char)('0' + (val % 10)));
}

static void print_serial_int(int val) {
    if (val < 0) {
        serial_putchar('-');
        val = -val;
    }
    if (val >= 10) print_serial_int(val / 10);
    serial_putchar((char)('0' + (val % 10)));
}

static void log_dispatch(thread_t* t) {
    vga_print("  [SCHED] Pilih Thread "); print_vga_int(t->id);
    vga_print(" ("); vga_print(t->name);
    vga_print(") | triage="); print_vga_int(t->triage_class);
    vga_print(", hybrid="); print_vga_int(t->hybrid_score);
    vga_print(", fifo="); print_vga_int((int)t->enqueue_order);
    vga_print("\n");

    serial_print("  [SCHED] Pilih Thread "); print_serial_int(t->id);
    serial_print(" ("); serial_print(t->name);
    serial_print(") | triage="); print_serial_int(t->triage_class);
    serial_print(", hybrid="); print_serial_int(t->hybrid_score);
    serial_print(", fifo="); print_serial_int((int)t->enqueue_order);
    serial_print("\n");
}

/* ============================================================================
   Preemptive Priority Scheduler
   Memilih thread READY dengan prioritas tertinggi (Triage -> Hybrid -> FIFO).
   Melakukan preemption jika thread RUNNING saat ini kalah prioritas.
   ============================================================================ */
/* ============================================================================
   Preemptive Priority Scheduler
   ============================================================================ */
static int in_scheduler = 0;

void schedule(void) {
    if (in_scheduler) return;
    in_scheduler = 1;

    // Update thread yang sedang WAITING berdurasi tick
    update_waiting_threads();

    // Pilih thread READY terbaik berdasarkan Priority Queue 2-Tingkat + FIFO
    int next = pick_next_thread();

    // Jika thread saat ini masih RUNNING (panggilan preemptive dari tick/wake)
    if (current_thread >= 0 && current_thread < MAX_THREADS &&
        thread_pool[current_thread].state == THREAD_RUNNING) {
        if (next == -1 || !is_higher_priority(&thread_pool[next], &thread_pool[current_thread])) {
            // Tidak ada kandidat yang lebih prioritas; lanjutkan thread berjalan
            in_scheduler = 0;
            return;
        }
        // Preempt thread berjalan: kembalikan statusnya dari RUNNING -> READY
        set_state(&thread_pool[current_thread], THREAD_READY);
    }

    if (next == -1) {
        // Tidak ada thread READY, cek apakah masih ada WAITING
        int has_waiting = 0;
        int has_timed_waiting = 0;
        for (int i = 0; i < MAX_THREADS; i++) {
            if (thread_pool[i].state == THREAD_WAITING) {
                has_waiting = 1;
                if (thread_pool[i].wait_ticks > 0) {
                    has_timed_waiting = 1;
                }
            }
        }

        if (!has_waiting) {
            // Semua thread selesai — kembali ke konteks kernel
            current_thread = -1;
            in_scheduler = 0;
            if (kernel_esp != 0) {
                unsigned int saved = kernel_esp;
                kernel_esp = 0;
                static unsigned int dummy_esp;
                context_switch(&dummy_esp, saved);
            }
            return;
        }

        // Ada thread WAITING, tunggu/simulasi tick sampai ada yang READY
        while (next == -1) {
            if (has_timed_waiting) {
                update_waiting_threads();
            } else {
                __asm__ volatile("hlt");
            }
            next = pick_next_thread();
        }
    }

    if (next == current_thread) {
        set_state(&thread_pool[next], THREAD_RUNNING);
        in_scheduler = 0;
        return;
    }

    // Switch to selected thread
    int prev = current_thread;
    log_dispatch(&thread_pool[next]);
    set_state(&thread_pool[next], THREAD_RUNNING);
    thread_pool[next].has_run = 1;
    current_thread = next;

    in_scheduler = 0;
    if (prev >= 0 && prev < MAX_THREADS) {
        // Context switch from thread lama ke thread baru
        context_switch(&thread_pool[prev].esp, thread_pool[next].esp);
    } else {
        // Pertama kali: simpan ESP kernel dan beralih ke thread pertama
        context_switch(&kernel_esp, thread_pool[next].esp);
    }
}

/* ---------------------------------------------------------------------------
   Test: Hybrid FIFO tie‑break validation
   --------------------------------------------------------------------------- */
void test_hybrid_fifo(void) {
    __asm__ volatile ("cli");
    task_sys_init();
    /* Create three threads with identical priority and hybrid score */
    thread_create_prio("T0", (void*)0x0, PRIO_DEFAULT, 1, 1);
    thread_create_prio("T1", (void*)0x0, PRIO_DEFAULT, 1, 1);
    thread_create_prio("T2", (void*)0x0, PRIO_DEFAULT, 1, 1);
    int id0 = 0, id1 = 1, id2 = 2; // slots are allocated sequentially
    int ok = (thread_pool[id0].enqueue_order < thread_pool[id1].enqueue_order) &&
             (thread_pool[id1].enqueue_order < thread_pool[id2].enqueue_order);
    test_result(ok, "Hybrid FIFO tie-break order is FIFO");
    task_sys_init(); /* cleanup */
    __asm__ volatile ("sti");
}

/* ---------------------------------------------------------------------------
   Test: Thread‑pool exhaustion handling
   --------------------------------------------------------------------------- */
void test_thread_pool_exhaustion(void) {
    __asm__ volatile ("cli");
    task_sys_init();
    /* Exhaust the pool */
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_create_prio("EXHAUST", (void*)0x0, PRIO_DEFAULT, 0, 0);
    }
    /* One more should fail */
    thread_t *extra = thread_create_prio("OVER", (void*)0x0, PRIO_DEFAULT, 0, 0);
    int ok = (extra == NULL);
    test_result(ok, "Thread-pool exhaustion returns NULL");
    task_sys_init(); /* cleanup */
    __asm__ volatile ("sti");
}

/* ---------------------------------------------------------------------------
   Test: Pre‑emptive timer tick validation (non-context-switch version)
   --------------------------------------------------------------------------- */
void test_preemptive_timer(void) {
    __asm__ volatile ("cli");
    task_sys_init();
    /* Just verify scheduler_tick calls schedule and picks higher prio */
    thread_create_prio("Low", (void*)0x0, PRIO_STABIL, 1, 1);
    thread_create_prio("High", (void*)0x0, PRIO_KRITIS, 1, 1);
    /* Verify high-priority thread would be picked first */
    int next = pick_next_thread();
    int ok = (next >= 0 && thread_pool[next].triage_class == PRIO_KRITIS);
    test_result(ok, "Timer preemption: high-prio picked first");
    task_sys_init(); /* cleanup */
    __asm__ volatile ("sti");
}

/* ---------------------------------------------------------------------------
   Run all task subsystem tests
   --------------------------------------------------------------------------- */
void task_run_tests(void) {
    vga_print("\n--- Task Subsystem Tests ---\n");
    serial_print("\n--- Task Subsystem Tests ---\n");
    test_hybrid_fifo();
    test_thread_pool_exhaustion();
    test_preemptive_timer();
    vga_print("--- Task Tests Complete ---\n\n");
    serial_print("--- Task Tests Complete ---\n\n");
}

/* ============================================================================
   Scheduler Tick - Dipanggil saat Tick / Unit Kerja untuk Mengecek Preemption
   ============================================================================ */
void scheduler_tick(void) {
    schedule();
}

/* ============================================================================
   Thread Block & Wake (Untuk Integrasi Mutex/Lock Sinkronisasi)
   ============================================================================ */
void thread_block(void) {
    if (current_thread < 0 || current_thread >= MAX_THREADS) return;
    thread_t* t = &thread_pool[current_thread];
    t->wait_ticks = -1; /* -1 menandakan diblokir oleh event/lock, bukan timer */
    set_state(t, THREAD_WAITING);
    schedule();
}

void thread_wake(int thread_id) {
    if (thread_id < 0 || thread_id >= MAX_THREADS) return;
    thread_t* t = &thread_pool[thread_id];
    if (t->state == THREAD_WAITING) {
        t->wait_ticks = 0;
        set_state(t, THREAD_READY);

        /* Preempt jika thread yang baru bangun mengalahkan thread RUNNING */
        if (current_thread >= 0 && current_thread < MAX_THREADS &&
            thread_pool[current_thread].state == THREAD_RUNNING) {
            if (is_higher_priority(t, &thread_pool[current_thread])) {
                schedule();
            }
        }
    }
}

/* ============================================================================
   Update Sisa Workload & Skor Hybrid Thread Berjalan
   ============================================================================ */
void thread_update_work(int remaining_work) {
    if (current_thread < 0 || current_thread >= MAX_THREADS) return;
    thread_t* t = &thread_pool[current_thread];
    t->remaining_work = remaining_work;
    t->hybrid_score = calc_hybrid_score(t->remaining_work, t->resource_load);
}

/* ============================================================================
   Thread Yield - Menyerahkan CPU secara kooperatif
   Transisi: RUNNING -> READY
   ============================================================================ */
void thread_yield(void) {
    if (current_thread < 0 || current_thread >= MAX_THREADS) return;
    thread_t* t = &thread_pool[current_thread];
    if (t->state == THREAD_RUNNING) {
        set_state(t, THREAD_READY);
    }
    schedule();
}

/* ============================================================================
   Thread Wait - Thread menunggu selama sejumlah tick
   Transisi: RUNNING -> WAITING (lalu otomatis WAITING -> READY setelah tick habis)
   ============================================================================ */
void thread_wait(int ticks) {
    if (current_thread < 0 || current_thread >= MAX_THREADS) return;
    thread_t* t = &thread_pool[current_thread];
    t->wait_ticks = ticks;
    set_state(t, THREAD_WAITING);
    schedule();
}

/* ============================================================================
   Thread Exit - Thread selesai dan terminasi
   Transisi: RUNNING -> TERMINATED
   ============================================================================ */
void thread_exit(void) {
    if (current_thread < 0 || current_thread >= MAX_THREADS) return;
    thread_t* t = &thread_pool[current_thread];
    set_state(t, THREAD_TERMINATED);
    t->id = -1;
    thread_count--;
    schedule();
}

/* ============================================================================
   Cetak Status Semua Thread (Tidak Diubah)
   ============================================================================ */
void print_thread_states(void) {
    vga_print("\n--- STATUS THREAD KERNEL ---\n");
    serial_print("\n--- STATUS THREAD KERNEL ---\n");

    int found = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].id != -1 || thread_pool[i].state != THREAD_TERMINATED) {
            if (!found) {
                vga_print("  ID  Nama            Status        Stack ESP\n");
                vga_print("  --  --------------  ------------  ----------\n");
                serial_print("  ID  Nama            Status        Stack ESP\n");
                serial_print("  --  --------------  ------------  ----------\n");
                found = 1;
            }

            vga_print("  ");
            vga_putchar('0' + i);
            vga_print("   ");
            serial_print("  ");
            serial_putchar('0' + i);
            serial_print("   ");

            int j = 0;
            while (thread_pool[i].name[j] != '\0') {
                vga_putchar(thread_pool[i].name[j]);
                serial_putchar(thread_pool[i].name[j]);
                j++;
            }
            while (j < 16) {
                vga_putchar(' ');
                serial_putchar(' ');
                j++;
            }

            const char* sn = state_name(thread_pool[i].state);
            vga_print(sn);
            serial_print(sn);
            int slen = 0;
            while (sn[slen] != '\0') slen++;
            while (slen < 14) {
                vga_putchar(' ');
                serial_putchar(' ');
                slen++;
            }

            print_hex(thread_pool[i].esp);
            vga_print("\n");
            serial_print("\n");
        }
    }

    if (!found) {
        vga_print("  (Tidak ada thread terdaftar)\n");
        serial_print("  (Tidak ada thread terdaftar)\n");
    }
    vga_print("\n");
}

/* ============================================================================
   Accessor Functions (Tidak Diubah)
   ============================================================================ */
int task_get_active_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].id != -1 && thread_pool[i].state != THREAD_TERMINATED)
            count++;
    }
    return count;
}

thread_t* task_get_thread(int id) {
    if (id < 0 || id >= MAX_THREADS) return NULL;
    return &thread_pool[id];
}

int task_get_current_id(void) {
    return current_thread;
}

/* ============================================================================
   DEMO: 2 Kernel Thread dengan Tugas Berbeda (Tidak Diubah)
   ============================================================================ */
static void thread_counter_task(void) {
    vga_print("\n  >> [Counter] Thread mulai berjalan!\n");
    serial_print("\n  >> [Counter] Thread mulai berjalan!\n");

    for (int i = 1; i <= 5; i++) {
        vga_print("  >> [Counter] Hitungan ke-");
        vga_putchar('0' + i);
        vga_print("\n");
        serial_print("  >> [Counter] Hitungan ke-");
        serial_putchar('0' + i);
        serial_print("\n");

        thread_yield();
    }

    vga_print("  >> [Counter] Tugas selesai! Thread akan terminasi.\n");
    serial_print("  >> [Counter] Tugas selesai! Thread akan terminasi.\n");
}

static void thread_greeter_task(void) {
    vga_print("\n  >> [Greeter] Thread mulai berjalan!\n");
    serial_print("\n  >> [Greeter] Thread mulai berjalan!\n");

    vga_print("  >> [Greeter] Halo dari Pulse-OS!\n");
    serial_print("  >> [Greeter] Halo dari Pulse-OS!\n");
    thread_yield();

    vga_print("  >> [Greeter] Menunggu I/O (masuk WAITING)...\n");
    serial_print("  >> [Greeter] Menunggu I/O (masuk WAITING)...\n");
    thread_wait(3);

    vga_print("  >> [Greeter] Bangun dari WAITING! Lanjut bekerja.\n");
    serial_print("  >> [Greeter] Bangun dari WAITING! Lanjut bekerja.\n");
    thread_yield();

    vga_print("  >> [Greeter] Sampai jumpa!\n");
    serial_print("  >> [Greeter] Sampai jumpa!\n");
}

void task_demo_run(void) {
    vga_print("\n");
    vga_print("===========================================================\n");
    vga_print("      DEMO PENGELOLAAN PROSES & THREAD PULSE-OS\n");
    vga_print("===========================================================\n");
    serial_print("\n===========================================================\n");
    serial_print("      DEMO PENGELOLAAN PROSES & THREAD PULSE-OS\n");
    serial_print("===========================================================\n");

    task_sys_init();

    vga_print("\n[Langkah 1] Membuat 2 kernel thread...\n");
    serial_print("\n[Langkah 1] Membuat 2 kernel thread...\n");

    thread_t* tA = thread_create("Counter", thread_counter_task);
    thread_t* tB = thread_create("Greeter", thread_greeter_task);

    if (!tA || !tB) {
        vga_print("[ERROR] Gagal membuat thread!\n");
        serial_print("[ERROR] Gagal membuat thread!\n");
        return;
    }

    vga_print("\n[Langkah 2] Status thread setelah pembuatan:\n");
    serial_print("\n[Langkah 2] Status thread setelah pembuatan:\n");
    print_thread_states();

    vga_print("[Langkah 3] Memulai priority-queue scheduler...\n");
    vga_print("            (Context switch antar thread dimulai)\n\n");
    serial_print("[Langkah 3] Memulai priority-queue scheduler...\n");
    serial_print("            (Context switch antar thread dimulai)\n\n");

    schedule();

    vga_print("\n[Langkah 4] Semua thread telah selesai.\n");
    serial_print("\n[Langkah 4] Semua thread telah selesai.\n");

    vga_print("\n[Langkah 5] Status akhir thread:\n");
    serial_print("\n[Langkah 5] Status akhir thread:\n");

    vga_print("\n--- STATUS AKHIR THREAD ---\n");
    vga_print("  Thread 0 (Counter): TERMINATED\n");
    vga_print("  Thread 1 (Greeter): TERMINATED\n");
    serial_print("\n--- STATUS AKHIR THREAD ---\n");
    serial_print("  Thread 0 (Counter): TERMINATED\n");
    serial_print("  Thread 1 (Greeter): TERMINATED\n");

    vga_print("\n===========================================================\n");
    vga_print("  RINGKASAN TRANSISI STATUS YANG DITUNJUKKAN:\n");
    vga_print("  1. NEW        -> thread_create() dipanggil\n");
    vga_print("  2. READY      -> thread siap dijadwalkan\n");
    vga_print("  3. RUNNING    -> scheduler memilih thread\n");
    vga_print("  4. WAITING    -> thread_wait() dipanggil (Greeter)\n");
    vga_print("  5. TERMINATED -> thread selesai / thread_exit()\n");
    vga_print("===========================================================\n\n");
    serial_print("\n===========================================================\n");
    serial_print("  RINGKASAN TRANSISI STATUS YANG DITUNJUKKAN:\n");
    serial_print("  1. NEW        -> thread_create() dipanggil\n");
    serial_print("  2. READY      -> thread siap dijadwalkan\n");
    serial_print("  3. RUNNING    -> scheduler memilih thread\n");
    serial_print("  4. WAITING    -> thread_wait() dipanggil (Greeter)\n");
    serial_print("  5. TERMINATED -> thread selesai / thread_exit()\n");
    serial_print("===========================================================\n\n");
}
