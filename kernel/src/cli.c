#include "../include/cli.h"
#include "../include/kernel.h"
#include "../include/memory.h"
#include "../include/sync.h"
#include "../include/task.h"
#include "../include/vfs.h"

void cli_print_logo(void) {
  vga_set_color(12, 0); // Warna Merah Terang (VGA color code 12)
  vga_print("========================================================\n");
  vga_print("       _     ___ _   _ _    ___ ___      ___  ___   \n");
  vga_print("     _| |_  | _ \\ | | | |  / __| __|___ / _ \\/ __|  \n");
  vga_print("    |_   _| |  _/ |_| | |__\\__ \\ _||___| (_) \\__ \\ \n");
  vga_print("      |_|   |_|  \\___/|____|___/___|    \\___/|___/  \n");
  vga_print("                                                        \n");
  vga_print("========================================================\n");
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

void cli_process_command(const char *cmd) {
  if (cmd[0] == '\0')
    return;

  cli_log_add(cmd);

  if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p') {
    vga_print("\nDaftar Perintah CLI:\n");
    vga_print("  help      - Menampilkan bantuan ini\n");
    vga_print("  version   - Menampilkan versi Pulse-OS\n");
    vga_print("  meminfo   - Status alokasi memori dinamis\n");
    vga_print("  ls        - List file di VFS RAMDisk\n");
    vga_print("  tasks     - Status thread & scheduler\n");
    vga_print("  clear     - Membersihkan layar\n");
    vga_print("  logs      - Menampilkan riwayat log aktivitas perintah\n");
    vga_print("  restart   - Merestart Pulse-OS\n");
    vga_print("  shutdown  - Mematikan sistem\n");
  } else if (cmd[0] == 'v' && cmd[1] == 'e' && cmd[2] == 'r' && cmd[3] == 's' &&
             cmd[4] == 'i' && cmd[5] == 'o' && cmd[6] == 'n') {
    vga_print("\nPulse-OS v1.0.0 (x86 Bare-Metal Kernel)\n");
  } else if (cmd[0] == 'm' && cmd[1] == 'e' && cmd[2] == 'm' && cmd[3] == 'i' &&
             cmd[4] == 'n' && cmd[5] == 'f' && cmd[6] == 'o') {
    print_memory_stats();
  } else if (cmd[0] == 'l' && cmd[1] == 's') {
    vfs_list();
  } else if (cmd[0] == 't' && cmd[1] == 'a' && cmd[2] == 's' && cmd[3] == 'k' &&
             cmd[4] == 's') {
    print_thread_states();
  } else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' &&
             cmd[4] == 'r') {
    vga_clear();
    cli_print_logo();
  } else if ((cmd[0] == 'l' && cmd[1] == 'o' && cmd[2] == 'g' && cmd[3] == 's') ||
             (cmd[0] == 'h' && cmd[1] == 'i' && cmd[2] == 's' && cmd[3] == 't' &&
              cmd[4] == 'o' && cmd[5] == 'r' && cmd[6] == 'y')) {
    cli_show_logs();
  } else if ((cmd[0] == 'r' || cmd[0] == 'R') &&
             (cmd[1] == 'e' || cmd[1] == 'E') &&
             (cmd[2] == 's' || cmd[2] == 'S') &&
             (cmd[3] == 't' || cmd[3] == 'T') &&
             (cmd[4] == 'a' || cmd[4] == 'A') &&
             (cmd[5] == 'r' || cmd[5] == 'R') &&
             (cmd[6] == 't' || cmd[6] == 'T')) {
    sys_restart();
  } else if ((cmd[0] == 's' && cmd[1] == 'h' && cmd[2] == 'u' && cmd[3] == 't' &&
              cmd[4] == 'd' && cmd[5] == 'o' && cmd[6] == 'w' && cmd[7] == 'n') ||
             (cmd[0] == 'p' && cmd[1] == 'o' && cmd[2] == 'w' && cmd[3] == 'e' &&
              cmd[4] == 'r' && cmd[5] == 'o' && cmd[6] == 'f' && cmd[7] == 'f')) {
    sys_shutdown();
  } else {
    vga_print("\nPerintah tidak dikenal. Ketik 'help'.\n");
  }
}

void cli_run(void) {
  cli_init();
  char buffer[64];
  int idx = 0;

  while (1) {
    vga_print("Pulse-OS> ");
    serial_print("Pulse-OS> ");
    idx = 0;

    while (1) {
      char c = input_getchar(); // Menerima input dari Keyboard PS/2 atau Serial
      if (c == '\r' || c == '\n') {
        buffer[idx] = '\0';
        vga_putchar('\n');
        serial_putchar('\n');
        break;
      } else if (c == '\b' && idx > 0) {
        idx--;
        vga_putchar('\b');
        serial_print("\b \b");
      } else if (idx < 63 && c >= 32 && c <= 126) {
        buffer[idx++] = c;
        vga_putchar(c);
        serial_putchar(c);
      }
    }

    cli_process_command(buffer);
  }
}
