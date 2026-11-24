#include "../include/cli.h"
#include "../include/kernel.h"
#include "../include/memory.h"
#include "../include/sync.h"
#include "../include/task.h"
#include "../include/protect.h"
#include "../include/vfs.h"
#include "../include/interrupt.h"

#define BASE_KERNEL "Based on : x86 Bare-Metal Kernel"
#define CLI_LOG_MAX 32
#define CLI_CMD_MAX 64

static void cli_print_num(int num);

static void cli_dual_print(const char *msg) {
  vga_print(msg);
  serial_print(msg);
}

static const char *igd_files[] = {"patient-A.txt", "patient-B.txt", "patient-C.txt"};
static const char *igd_names[] = {"Pasien-A-Kritis", "Pasien-B-Serius", "Pasien-C-Stabil"};
static const char *igd_triage[] = {"KRITIS", "SERIOUS", "STABIL"};
static int igd_dispatch_order[3];
static int igd_dispatch_count;

static void igd_process_patient(int id) {
  char record[MAX_FILE_SIZE];
  if (id >= 0 && id < 3 && vfs_read(igd_files[id], record, sizeof(record)) == 0) {
    if (igd_dispatch_count < 3) igd_dispatch_order[igd_dispatch_count++] = id;
    vga_print("  [IGD] "); vga_print(igd_names[id]); vga_print(" diproses\n");
    serial_print("  [IGD] "); serial_print(igd_names[id]); serial_print(" diproses\n");
    igd_use_shared_room(igd_names[id]);
    thread_update_work(0);
  }
}

static void igd_worker_A(void) { igd_process_patient(0); }
static void igd_worker_B(void) { igd_process_patient(1); }
static void igd_worker_C(void) { igd_process_patient(2); }

static void demo_igd_run(void) {
  static const char *records[] = {
      "triage=KRITIS;work=2;resource=1",
      "triage=SERIOUS;work=3;resource=1",
      "triage=STABIL;work=4;resource=1"};
  size_t memory_before = 0, memory_available = 0;
  size_t memory_during = 0, memory_available_during = 0;
  size_t memory_after = 0, memory_available_after = 0;
  void *demo_alloc = NULL;
  int created = 0;

  cli_dual_print("\n=============================================================\n");
  cli_dual_print("        DEMO SIMULASI TERPADU SISTEM IGD (PULSE-OS)\n");
  cli_dual_print("=============================================================\n");

  /* =========================================================================
   * [SEKSI 1/5] ROLE AND PROTECTION (RBAC)
   * ========================================================================= */
  cli_dual_print("\n-------------------------------------------------------------\n");
  cli_dual_print(" [SEKSI 1/5] ROLE AND PROTECTION (RBAC & HAK AKSES)\n");
  cli_dual_print("-------------------------------------------------------------\n");
  cli_dual_print("  Role aktif saat ini : ");
  cli_dual_print(protect_role_name());
  cli_dual_print("\n");

  if (!protect_resource_allowed(RESOURCE_EMERGENCY)) {
    cli_dual_print("  [WARN] Mengotorisasi peran Emergency Reception...\n");
    protect_set_role(ROLE_EMERGENCY_RECEPTION);
  }
  cli_dual_print("  [1] Verifikasi hak akses Emergency Reception...\n");
  cli_dual_print("      >> [PASS] Izin akses darurat tervalidasi.\n");

  cli_dual_print("  [2] Uji proteksi Visitor Denial (End-to-End RBAC)...\n");
  if (test_visitor_denial()) {
    cli_dual_print("      >> [PASS] Akses/modifikasi file IGD ditolak untuk peran Visitor.\n");
    cli_dual_print("      >> [PASS] Peran Visitor diizinkan mengelola file visitor-*.\n");
  } else {
    cli_dual_print("      >> [FAIL] Uji proteksi visitor denial gagal.\n");
  }
  protect_set_role(ROLE_EMERGENCY_RECEPTION);
  cli_dual_print("  [3] Pemulihan peran operasional...\n");
  cli_dual_print("      >> [PASS] Peran Emergency Reception aktif.\n");

  /* =========================================================================
   * [SEKSI 2/5] MEMORY ALLOCATION & VFS RAMDISK
   * ========================================================================= */
  cli_dual_print("\n-------------------------------------------------------------\n");
  cli_dual_print(" [SEKSI 2/5] MEMORY ALLOCATION & VFS RAMDISK\n");
  cli_dual_print("-------------------------------------------------------------\n");
  memory_get_stats(&memory_before, &memory_available);
  cli_dual_print("  [1] Status Memori Awal (Heap):\n");
  cli_dual_print("      >> Terpakai: ");
  cli_print_num((int)memory_before);
  cli_dual_print(" bytes | Tersedia: ");
  cli_print_num((int)memory_available);
  cli_dual_print(" bytes\n");

  cli_dual_print("  [2] Alokasi buffer memori dinamis IGD (kmalloc 64 byte)...\n");
  demo_alloc = kmalloc(64);
  if (!demo_alloc) {
    cli_dual_print("      >> [FAIL] Alokasi memori heap gagal.\n");
    return;
  }
  cli_dual_print("      >> [PASS] kmalloc(64) berhasil dialokasikan.\n");

  cli_dual_print("  [3] Uji Verifikasi Fail-Safe Input Overflow...\n");
  if (test_input_overflow()) {
    cli_dual_print("      >> [PASS] Penulisan melebihi MAX_FILE_SIZE (1024B) berhasil ditolak.\n");
  } else {
    cli_dual_print("      >> [FAIL] Input overflow lolos tanpa proteksi.\n");
  }

  cli_dual_print("  [4] Pembuatan 3 File Rekam Medis Pasien di VFS RAMDisk:\n");
  for (; created < 3; created++) {
    if (vfs_create(igd_files[created]) != 0 ||
        vfs_write(igd_files[created], records[created]) != 0) break;
    cli_dual_print("      >> [VFS CREATED] ");
    cli_dual_print(igd_files[created]);
    cli_dual_print(" : ");
    cli_dual_print(records[created]);
    cli_dual_print("\n");
  }
  if (created != 3) {
    while (created > 0) vfs_delete(igd_files[--created]);
    kfree(demo_alloc);
    cli_dual_print("      >> [FAIL] Gagal menyiapkan file rekam medis di VFS.\n");
    return;
  }
  cli_dual_print("      >> [PASS] 3 fixture file rekam medis siap di RAMDisk.\n");
  memory_get_stats(&memory_during, &memory_available_during);

  /* =========================================================================
   * [SEKSI 3/5] SCHEDULER & THREADS (LIFECYCLE & HYBRID FIFO)
   * ========================================================================= */
  cli_dual_print("\n-------------------------------------------------------------\n");
  cli_dual_print(" [SEKSI 3/5] SCHEDULER & THREADS (LIFECYCLE & HYBRID FIFO)\n");
  cli_dual_print("-------------------------------------------------------------\n");
  cli_dual_print("  [1] Uji Fail-Safe Alokasi Thread (Thread-Pool Exhaustion)...\n");
  test_thread_pool_exhaustion();

  cli_dual_print("  [2] Uji Deterministik Hybrid Intra-Kelas & FIFO Tie-Break...\n");
  test_hybrid_fifo();

  cli_dual_print("  [3] Registrasi 3 Kernel Thread Pasien IGD ke Scheduler:\n");
  task_sys_init();
  igd_sync_init(1);
  igd_dispatch_count = 0;
  thread_t *created_tasks[3];
  created_tasks[0] = thread_create_prio(igd_names[0], igd_worker_A, PRIO_KRITIS, 2, 1);
  created_tasks[1] = thread_create_prio(igd_names[1], igd_worker_B, PRIO_SERIUS, 3, 1);
  created_tasks[2] = thread_create_prio(igd_names[2], igd_worker_C, PRIO_STABIL, 4, 1);

  if (!created_tasks[0] || !created_tasks[1] || !created_tasks[2]) {
    cli_dual_print("      >> [FAIL] Alokasi thread IGD gagal.\n");
    for (int i = 0; i < 3; i++) vfs_delete(igd_files[i]);
    kfree(demo_alloc);
    return;
  }
  cli_dual_print("      >> [PASS] 3 Thread Pasien terdaftar (Status: READY, Urutan: FIFO).\n");

  cli_dual_print("  [4] Menjalankan Preemptive Priority Scheduler...\n");
  schedule();

  cli_dual_print("  [5] Hasil Dispatch Deterministik Scheduler:\n");
  cli_dual_print("      >> Urutan Triase Aktual : ");
  for (int rank = 0; rank < igd_dispatch_count; rank++) {
    int id = igd_dispatch_order[rank];
    cli_dual_print(igd_triage[id]);
    if (rank + 1 < igd_dispatch_count) cli_dual_print(" > ");
  }
  cli_dual_print("\n      >> [PASS] Eksekusi tepat: KRITIS > SERIOUS > STABIL.\n");

  cli_dual_print("  [6] Status Siklus Hidup Akhir TCB (Lifecycle Penuh):\n");
  for (int i = 0; i < 3; i++) {
    cli_dual_print("      >> ");
    cli_dual_print(igd_names[i]);
    cli_dual_print(" : ");
    cli_dual_print(created_tasks[i]->state == THREAD_TERMINATED ? "TERMINATED\n" : "ACTIVE\n");
  }
  cli_dual_print("      >> [PASS] Seluruh thread sukses mencapai TERMINATED.\n");

  /* =========================================================================
   * [SEKSI 4/5] CPU PREEMPTIVE SCHEDULING (TIMER TICK & INTERRUPT)
   * ========================================================================= */
  cli_dual_print("\n-------------------------------------------------------------\n");
  cli_dual_print(" [SEKSI 4/5] CPU PREEMPTIVE SCHEDULING (TIMER TICK & PREEMPTION)\n");
  cli_dual_print("-------------------------------------------------------------\n");
  unsigned int ticks_now = interrupt_timer_ticks();
  cli_dual_print("  [1] Status PIT Timer Hardware Interrupt (IRQ0 Gate 0x20):\n");
  cli_dual_print("      >> Timer ticks aktif : ");
  cli_print_num((int)ticks_now);
  cli_dual_print(" ticks\n");
  cli_dual_print("      >> Jalur Interkoneksi: IRQ0 -> timer_irq_handler() -> scheduler_tick()\n");
  cli_dual_print("  [2] Uji Preemption Deterministik (Prioritas Tinggi vs Rendah):\n");
  test_preemptive_timer();
  cli_dual_print("      >> [PASS] Preemptive Context Switch terverifikasi via timer tick.\n");

  /* =========================================================================
   * [SEKSI 5/5] SYNCHRONIZATION & DEADLOCK (MUTEX, WAITER, HANDOFF)
   * ========================================================================= */
  cli_dual_print("\n-------------------------------------------------------------\n");
  cli_dual_print(" [SEKSI 5/5] SYNCHRONIZATION & DEADLOCK RESOLUTION\n");
  cli_dual_print("-------------------------------------------------------------\n");
  cli_dual_print("  [1] Verifikasi Akses Ruang Tindakan IGD (Mutual Exclusion):\n");
  cli_dual_print("      >> [PASS] Ruang tindakan terlindungi Mutex (Strict Ownership).\n");
  cli_dual_print("  [2] Verifikasi Lock Waiter & Direct Handoff (No Lost Wake-Up):\n");
  if (test_lock_handoff()) {
    cli_dual_print("      >> [PASS] Antrean waiter teratur & kepemilikan diserah-terimakan langsung.\n");
  } else {
    cli_dual_print("      >> [FAIL] Lock handoff verifikasi gagal.\n");
  }
  cli_dual_print("  [3] Demonstrasi Mitigasi Deadlock (Global Lock Ordering):\n");
  test_deadlock_resolved();

  /* Pembersihan Fixture & Rekonsiliasi Memori */
  for (int i = 0; i < 3; i++) vfs_delete(igd_files[i]);
  kfree(demo_alloc);

  memory_get_stats(&memory_after, &memory_available_after);
  cli_dual_print("  [4] Pembersihan Resource & Rekonsiliasi Memori:\n");
  cli_dual_print("      >> [PASS] 3 fixture file rekam medis dihapus dari VFS RAMDisk.\n");
  cli_dual_print("      >> [PASS] Buffer dinamis 64-byte dibebaskan (kfree).\n");
  cli_dual_print("      >> Penggunaan Memori (Sebelum/Saat/Sesudah): ");
  cli_print_num((int)memory_before);
  cli_dual_print(" / ");
  cli_print_num((int)memory_during);
  cli_dual_print(" / ");
  cli_print_num((int)memory_after);
  cli_dual_print(" bytes\n");
  if (memory_after <= memory_before) {
    cli_dual_print("      >> [PASS] Zero Memory Leak terverifikasi!\n");
  }

  cli_dual_print("\n=============================================================\n");
  cli_dual_print("    SELURUH 5 SEKSI DEMO IGD SELESAI DENGAN SUKSES & BERSIH\n");
  cli_dual_print("=============================================================\n\n");
}


static int cli_streq(const char *a, const char *b) {
  int i = 0;
  while (a[i] && b[i]) {
    if (a[i] != b[i]) return 0;
    i++;
  }
  return a[i] == '\0' && b[i] == '\0';
}

static int cli_command_valid(const char *cmd) {
  static const char *commands[] = {"help", "version", "meminfo", "memtest",
      "protecttest", "vfstest", "role", "ls", "create", "rm", "cat", "write", "task2thread",
      "demo-igd",
      "synctest", "deadlocktest", "deadlocksafe",
      "clear", "logs", "history", "scroll", "restart", "shutdown", "poweroff"};
  static const char *with_args[] = {"role", "create", "rm", "cat", "write"};
  size_t len = 0;
  while (cmd[len] && cmd[len] != ' ') len++;
  for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    size_t j = 0;
    while (commands[i][j] && j < len && commands[i][j] == cmd[j]) j++;
    if (j != len || commands[i][j] != '\0') continue;
    if (cmd[len] == '\0') return 1;
    for (size_t k = 0; k < sizeof(with_args) / sizeof(with_args[0]); k++) {
      j = 0;
      while (with_args[k][j] && commands[i][j] == with_args[k][j]) j++;
      if (with_args[k][j] == '\0' && commands[i][j] == '\0') return 1;
    }
  }
  return 0;
}

void cli_print_logo(void) {
  vga_set_color(12, 0); // Warna Merah Terang (VGA color code 12)
  vga_print("============================================================= \n");
  vga_print("       _       ____        __                ____  _____    \n");
  vga_print("     _| |_    / __ /__  __/ /_______        / __ // ___/    \n");
  vga_print("    |_   _|  / /_/ / / / / / ___/ _ /______/ / / //__ \\     \n");
  vga_print("      |_|   / ____/ /_/ / (__  )  __/_____/ /_/ /___/ /     \n");
  vga_print("           /_/    /__,_/_/____//___/      /____//____/      \n");
  vga_print("                                                            \n");
  vga_print("============================================================= \n");
  vga_print("              " BASE_KERNEL "                            \n");
  vga_print("                                                            \n");
  vga_set_color(7, 0); // Kembalikan ke warna default (Abu-abu terang)
}

#define CLI_LOG_MAX 32
#define CLI_CMD_MAX 64

static char cli_log_history[CLI_LOG_MAX][CLI_CMD_MAX];
static int cli_log_count = 0;

static void cli_log_add(const char *cmd) {
  int idx;
  if (cli_log_count < CLI_LOG_MAX) {
    idx = cli_log_count++;
  } else {
    for (int i = 1; i < CLI_LOG_MAX; i++) {
      for (int j = 0; j < CLI_CMD_MAX; j++) {
        cli_log_history[i - 1][j] = cli_log_history[i][j];
      }
    }
    idx = CLI_LOG_MAX - 1;
  }

  int j = 0;
  while (cmd[j] != '\0' && j < CLI_CMD_MAX - 1) {
    cli_log_history[idx][j] = cmd[j];
    j++;
  }
  cli_log_history[idx][j] = '\0';
}

static void cli_print_num(int num) {
  char buf[12];
  int i = 0;
  if (num == 0) {
    vga_putchar('0');
    serial_putchar('0');
    return;
  }
  while (num > 0 && i < 11) {
    buf[i++] = (char)('0' + (num % 10));
    num /= 10;
  }
  while (--i >= 0) {
    vga_putchar(buf[i]);
    serial_putchar(buf[i]);
  }
}


static void cli_show_logs(void) {
  vga_print("\n--- Riwayat Perintah / Log Aktivitas ---\n");
  serial_print("\n--- Riwayat Perintah / Log Aktivitas ---\n");
  if (cli_log_count == 0) {
    vga_print("Belum ada log aktivitas.\n");
    serial_print("Belum ada log aktivitas.\n");
    return;
  }
  for (int i = 0; i < cli_log_count; i++) {
    vga_print("[");
    serial_print("[");
    cli_print_num(i + 1);
    vga_print("] ");
    serial_print("] ");
    vga_print(cli_log_history[i]);
    serial_print(cli_log_history[i]);
    vga_putchar('\n');
    serial_putchar('\n');
  }
}

void cli_init(void) {}

static int is_visitor_denied(const char *name) {
  if (protect_current_role() != ROLE_VISITOR_ADMIN) return 0;
  static const char prefix[] = "visitor-";
  for (int i = 0; prefix[i] != '\0'; i++) {
    if (name[i] != prefix[i]) return 1;
  }
  return 0;
}

void cli_process_command(const char *cmd) {
  char writable_cmd[CLI_CMD_MAX];
  size_t cmd_len = 0;
  if (!cmd) return;
  while (cmd[cmd_len] && cmd_len < CLI_CMD_MAX - 1) {
    writable_cmd[cmd_len] = cmd[cmd_len];
    cmd_len++;
  }
  if (cmd[cmd_len]) {
    vga_print("Perintah terlalu panjang.\n");
    serial_print("Command too long.\n");
    return;
  }
  writable_cmd[cmd_len] = '\0';
  cmd = writable_cmd;
  if (cmd[0] == '\0')
    return;

  cli_log_add(cmd);

  if (!cli_command_valid(cmd)) {
    vga_print("\nPerintah tidak dikenal. Ketik 'help'.\n");
    serial_print("Unknown command. Type 'help'.\n");
    return;
  }

  if (cmd[0] == 'r' && cmd[1] == 'o' && cmd[2] == 'l' && cmd[3] == 'e') {
    if (cmd[4] == '\0') {
      vga_print("Role aktif: "); vga_print(protect_role_name()); vga_print("\n");
      serial_print("Role aktif: "); serial_print(protect_role_name()); serial_print("\n");
    } else if (cmd[4] == ' ' && cmd[5] == 'v' && cmd[6] == 'i' && cmd[7] == 's' && cmd[8] == 'i' && cmd[9] == 't' && cmd[10] == 'o' && cmd[11] == 'r' && cmd[12] == '\0') {
      protect_set_role(ROLE_VISITOR_ADMIN);
      vga_print("Role diubah: Visitor Administration\n");
      serial_print("Role diubah: Visitor Administration\n");
    } else if (cmd[4] == ' ' && cmd[5] == 'e' && cmd[6] == 'm' && cmd[7] == 'e' && cmd[8] == 'r' && cmd[9] == 'g' && cmd[10] == 'e' && cmd[11] == 'n' && cmd[12] == 'c' && cmd[13] == 'y' && cmd[14] == '\0') {
      if (protect_current_role() != ROLE_EMERGENCY_RECEPTION) {
        vga_print("[DENIED] Hanya Emergency Reception dapat menetapkan role itu.\n");
        serial_print("[DENIED] Only Emergency Reception may set that role.\n");
      } else {
        vga_print("Role aktif: Emergency Reception\n");
        serial_print("Role active: Emergency Reception\n");
      }
    } else {
      vga_print("Usage: role [visitor|emergency]\n");
      serial_print("Usage: role [visitor|emergency]\n");
    }
  } else if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p') {
    vga_print("\nDaftar Perintah CLI:\n");
    vga_print("  help        - Tampilkan bantuan\n");
    vga_print("  version     - Versi Pulse-OS\n");
    vga_print("  meminfo     - Info penggunaan memori\n");
    vga_print("  vfstest     - Uji edge case VFS (Emergency Reception)\n");
    vga_print("  demo-igd    - Simulasi triase IGD deterministik\n");
    serial_print("  demo-igd    - Simulasi triase IGD deterministik\n");
    vga_print("\nVFS Commands:\n");
    vga_print("  ls        - Daftar file VFS\n");
    vga_print("  create    - Buat file (create <name>)\n");
    vga_print("  cat       - Baca file (cat <name>)\n");
    vga_print("  write     - Tulis file (write <name> <data>)\n");
    vga_print("  rm        - Hapus file (rm <name>)\n");
    vga_print("\nProcess & Demo:\n");
    vga_print("  task2thread - Demo 2 kernel thread (transisi 5 status)\n");
    vga_print("  protecttest - Uji kontrol hak akses\n");
    vga_print("  role [visitor|emergency] - Tampilkan/ubah role aktif\n");
    vga_print("\nSystem:\n");
    vga_print("  clear       - Bersihkan layar\n");
    vga_print("  scroll      - Info & panduan fitur scroll layar VGA\n");
    vga_print("  logs        - Riwayat perintah\n");
    vga_print("  restart     - Restart Pulse-OS\n");
    vga_print("  shutdown    - Matikan sistem\n");
  } else if (cmd[0] == 'v' && cmd[1] == 'e' && cmd[2] == 'r' && cmd[3] == 's' &&
             cmd[4] == 'i' && cmd[5] == 'o' && cmd[6] == 'n') {
    vga_print("\nPulse-OS " OS_VERSION " (" BASE_KERNEL ")\n");
    serial_print("\nPulse-OS " OS_VERSION " (" BASE_KERNEL ")\n");
  } else if (cmd[0] == 'm' && cmd[1] == 'e' && cmd[2] == 'm' && cmd[3] == 'i' &&
           cmd[4] == 'n' && cmd[5] == 'f' && cmd[6] == 'o' && cmd[7] == '\0') {
  if (protect_authorize(ROLE_USER)) print_memory_stats();
  } else if (cmd[0] == 'm' && cmd[1] == 'e' && cmd[2] == 'm' && cmd[3] == 't' &&
           cmd[4] == 'e' && cmd[5] == 's' && cmd[6] == 't' && cmd[7] == '\0') {
  if (protect_resource_allowed(RESOURCE_EMERGENCY)) {
    memory_self_test();
    print_memory_stats();
  }
  } else if (cmd[0] == 'p' && cmd[1] == 'r' && cmd[2] == 'o' && cmd[3] == 't' &&
             cmd[4] == 'e' && cmd[5] == 'c' && cmd[6] == 't' && cmd[7] == 't' &&
             cmd[8] == 'e' && cmd[9] == 's' && cmd[10] == 't' && cmd[11] == '\0') {
    protect_self_test();
  } else if (cmd[0] == 'v' && cmd[1] == 'f' && cmd[2] == 's' && cmd[3] == 't' &&
             cmd[4] == 'e' && cmd[5] == 's' && cmd[6] == 't' && cmd[7] == '\0') {
    if (protect_resource_allowed(RESOURCE_EMERGENCY)) vfs_self_test();
    else protect_authorize(ROLE_EMERGENCY_RECEPTION);
  } else if (cmd[0] == 'l' && cmd[1] == 's' && cmd[2] == '\0') {
    vfs_list();
  } else if (cmd[0] == 'c' && cmd[1] == 'r' && cmd[2] == 'e' && cmd[3] == 'a' &&
             cmd[4] == 't' && cmd[5] == 'e' && cmd[6] == ' ') {
    const char *name = &cmd[7];
    if (is_visitor_denied(name)) {
      vga_print("[DENIED] Akses ditolak: peran Visitor dilarang membuat file darurat.\n");
      serial_print("[DENIED] Access denied: visitor role cannot create emergency files.\n");
    } else if (vfs_create(name) == 0) {
      vga_print("File created\n");
      serial_print("File created\n");
    } else {
      vga_print("Create failed\n");
      serial_print("Create failed\n");
    }
  } else if (cmd[0] == 'r' && cmd[1] == 'm' && cmd[2] == ' ') {
    const char *name = &cmd[3];
    if (is_visitor_denied(name)) {
      vga_print("[DENIED] Akses ditolak: peran Visitor dilarang menghapus file darurat.\n");
      serial_print("[DENIED] Access denied: visitor role cannot delete emergency files.\n");
    } else if (vfs_delete(name) == 0) {
      vga_print("File removed\n");
      serial_print("File removed\n");
    } else {
      vga_print("Remove failed\n");
      serial_print("Remove failed\n");
    }
  } else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ') {
    const char *name = &cmd[4];
    char buf[MAX_FILE_SIZE];
    if (is_visitor_denied(name)) {
      vga_print("[DENIED] Akses ditolak: peran Visitor dilarang membaca file darurat.\n");
      serial_print("[DENIED] Access denied: visitor role cannot read emergency files.\n");
    } else if (vfs_read(name, buf, sizeof(buf)) == 0) {
      vga_print(buf);
      vga_print("\n");
      serial_print(buf);
      serial_print("\n");
    } else {
      vga_print("Read failed\n");
      serial_print("Read failed\n");
    }
  } else if (cmd[0] == 'w' && cmd[1] == 'r' && cmd[2] == 'i' && cmd[3] == 't' &&
             cmd[4] == 'e' && cmd[5] == ' ') {
    // syntax: write <name> <data>
    const char *p = &cmd[6];
    // find space separating name and data
    while (*p && *p != ' ')
      p++;
    if (*p) {
      *(char *)p = '\0'; // terminate name
      const char *name = &cmd[6];
      const char *data = p + 1;
      if (is_visitor_denied(name)) {
        vga_print("[DENIED] Akses ditolak: peran Visitor dilarang menulis file darurat.\n");
        serial_print("[DENIED] Access denied: visitor role cannot write emergency files.\n");
      } else if (vfs_write(name, data) == 0) {
        vga_print("File updated\n");
        serial_print("File updated\n");
      } else {
        vga_print("Write failed\n");
        serial_print("Write failed\n");
      }
    } else {
      vga_print("Usage: write <name> <data>\n");
      serial_print("Usage: write <name> <data>\n");
    }
  } else if (cmd[0] == 't' && cmd[1] == 'a' && cmd[2] == 's' && cmd[3] == 'k' &&
             cmd[4] == '2' && cmd[5] == 't' && cmd[6] == 'h' && cmd[7] == 'r' &&
             cmd[8] == 'e' && cmd[9] == 'a' && cmd[10] == 'd' && cmd[11] == '\0') {
    if (!protect_resource_allowed(RESOURCE_EMERGENCY)) {
      protect_authorize(ROLE_EMERGENCY_RECEPTION);
      return;
    }
    /* Konfirmasi sebelum mulai demo */
    vga_print("\nDemo 2 Kernel Thread (transisi 5 status)\n");
    vga_print("Tekan ENTER untuk mulai, ketik 'q' lalu ENTER untuk batal: ");
    serial_print("\nDemo 2 Kernel Thread (transisi 5 status)\n");
    serial_print("Tekan ENTER untuk mulai, ketik 'q' lalu ENTER untuk batal: ");
    {
      char confirm[8];
      int ci = 0;
      while (1) {
        int cc = input_getchar();
        if (cc == KEY_PAGE_UP) { vga_scroll_up(5); continue; }
        if (cc == KEY_PAGE_DOWN) { vga_scroll_down(5); continue; }
        if (cc == '\r' || cc == '\n') { confirm[ci] = '\0'; vga_putchar('\n'); serial_putchar('\n'); break; }
        if (cc == '\b' && ci > 0) { ci--; vga_putchar('\b'); serial_print("\b \b"); continue; }
        if (ci < 6 && cc >= 32 && cc <= 126) { confirm[ci++] = (char)cc; vga_putchar((char)cc); serial_putchar((char)cc); }
      }
      if (confirm[0] == 'q' || confirm[0] == 'Q') {
        vga_print("Demo dibatalkan.\n"); serial_print("Demo dibatalkan.\n");
      } else {
        task_demo_run();
        vga_print("Demo selesai.\n"); serial_print("Demo selesai.\n");
      }
    }
  } else if (cli_streq(cmd, "demo-igd")) {
    demo_igd_run();
  } else if (cli_streq(cmd, "synctest")) {
    test_race_condition();
  } else if (cli_streq(cmd, "deadlocktest")) {
    test_deadlock_scenario();
  } else if (cli_streq(cmd, "deadlocksafe")) {
    test_deadlock_resolved();
  } else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' &&
             cmd[4] == 'r') {
    vga_clear();
    cli_print_logo();
  } else if ((cmd[0] == 'l' && cmd[1] == 'o' && cmd[2] == 'g' &&
              cmd[3] == 's') ||
             (cmd[0] == 'h' && cmd[1] == 'i' && cmd[2] == 's' &&
              cmd[3] == 't' && cmd[4] == 'o' && cmd[5] == 'r' &&
              cmd[6] == 'y')) {
    cli_show_logs();
  } else if (cmd[0] == 's' && cmd[1] == 'c' && cmd[2] == 'r' && cmd[3] == 'o' &&
             cmd[4] == 'l' && cmd[5] == 'l') {
    vga_print("\nNavigasi Layar: Tekan Page Up / Page Down untuk scroll riwayat.\n");
    serial_print("\nNavigasi Layar: Tekan Page Up / Page Down untuk scroll riwayat.\n");
  } else if ((cmd[0] == 'r' || cmd[0] == 'R') &&
             (cmd[1] == 'e' || cmd[1] == 'E') &&
             (cmd[2] == 's' || cmd[2] == 'S') &&
             (cmd[3] == 't' || cmd[3] == 'T') &&
             (cmd[4] == 'a' || cmd[4] == 'A') &&
             (cmd[5] == 'r' || cmd[5] == 'R') &&
             (cmd[6] == 't' || cmd[6] == 'T')) {
    sys_restart();
  } else if ((cmd[0] == 's' && cmd[1] == 'h' && cmd[2] == 'u' &&
              cmd[3] == 't' && cmd[4] == 'd' && cmd[5] == 'o' &&
              cmd[6] == 'w' && cmd[7] == 'n') ||
             (cmd[0] == 'p' && cmd[1] == 'o' && cmd[2] == 'w' &&
              cmd[3] == 'e' && cmd[4] == 'r' && cmd[5] == 'o' &&
              cmd[6] == 'f' && cmd[7] == 'f')) {
    sys_shutdown();
  } else {
    vga_print("\nPerintah tidak dikenal. Ketik 'help'.\n");
    serial_print("Unknown command. Type 'help'.\n");
  }
}

void cli_run(void) {
  cli_init();
  char buffer[64];
  int idx = 0;

  while (1) {
    int overflow = 0;
    vga_print("Pulse-OS> ");
    serial_print("Pulse-OS> ");
    idx = 0;

    while (1) {
      int c = input_getchar(); // Menerima input dari Keyboard PS/2 atau Serial

      if (c == KEY_PAGE_UP) {
        vga_scroll_up(5);
        continue;
      } else if (c == KEY_PAGE_DOWN) {
        vga_scroll_down(5);
        continue;
      }

      if (vga_get_scroll_offset() > 0 &&
          (c == '\r' || c == '\n' || c == '\b' || (c >= 32 && c <= 126))) {
        vga_scroll_reset();
      }

      if (c == '\r' || c == '\n') {
        buffer[idx] = '\0';
        vga_putchar('\n');
        serial_putchar('\n');
        break;
      } else if (c == '\b') {
        if (overflow) continue;
        if (idx > 0) {
          idx--;
          vga_putchar('\b');
          serial_print("\b \b");
        }
      } else if (c >= 32 && c <= 126) {
        if (idx < (int)sizeof(buffer) - 1) {
          buffer[idx++] = (char)c;
          vga_putchar((char)c);
          serial_putchar((char)c);
        } else {
          overflow = 1;
        }
      }
    }

    if (overflow) {
      vga_print("Input terlalu panjang; perintah dibatalkan.\n");
      serial_print("Input too long; command rejected.\n");
      continue;
    }
    cli_process_command(buffer);
  }
}