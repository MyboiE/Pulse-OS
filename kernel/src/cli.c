#include "../include/cli.h"
#include "../include/kernel.h"
#include "../include/memory.h"
#include "../include/sync.h"
#include "../include/task.h"
#include "../include/vfs.h"

#define BASE_KERNEL "Based on : x86 Bare-Metal Kernel"

void cli_print_logo(void) {
  vga_set_color(12, 0); // Warna Merah Terang (VGA color code 12)
  vga_print("============================================================= \n");
  vga_print("       _       ____        __                ____  _____    \n");
  vga_print("     _| |_    / __ /__  __/ /_______        / __ // ___/    \n");
  vga_print("    |_   _|  / /_/ / / / / / ___/ _ /______/ / / //__ \     \n");
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

void cli_process_command(const char *cmd) {
  if (cmd[0] == '\0')
    return;

  cli_log_add(cmd);

  if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p') {
    vga_print("\nDaftar Perintah CLI:\n");
    vga_print("  help      - Tampilkan bantuan\n");
    vga_print("  version   - Versi Pulse-OS\n");
    vga_print("  meminfo   - Info penggunaan memori\n");
    vga_print("\nVFS Commands:\n");
    vga_print("  ls        - Daftar file VFS\n");
    vga_print("  create    - Buat file (create <name>)\n");
    vga_print("  cat       - Baca file (cat <name>)\n");
    vga_print("  write     - Tulis file (write <name> <data>)\n");
    vga_print("  rm        - Hapus file (rm <name>)\n");
    vga_print("\nProcess & Thread:\n");
    vga_print("  task2thread - Demo 2 kernel thread (transisi 5 status)\n");
    vga_print("\nSystem:\n");
    vga_print("  clear       - Bersihkan layar\n");
    vga_print("  logs        - Riwayat perintah\n");
    vga_print("  restart     - Restart Pulse-OS\n");
    vga_print("  shutdown    - Matikan sistem\n");
  } else if (cmd[0] == 'v' && cmd[1] == 'e' && cmd[2] == 'r' && cmd[3] == 's' &&
             cmd[4] == 'i' && cmd[5] == 'o' && cmd[6] == 'n') {
    vga_print("\nPulse-OS " OS_VERSION " (" BASE_KERNEL ")\n");
  } else if (cmd[0] == 'm' && cmd[1] == 'e' && cmd[2] == 'm' && cmd[3] == 'i' &&
             cmd[4] == 'n' && cmd[5] == 'f' && cmd[6] == 'o') {
    print_memory_stats();
  } else if (cmd[0] == 'l' && cmd[1] == 's') {
    vfs_list();
  } else if (cmd[0] == 'c' && cmd[1] == 'r' && cmd[2] == 'e' && cmd[3] == 'a' &&
             cmd[4] == 't' && cmd[5] == 'e' && cmd[6] == ' ') {
    const char *name = &cmd[7];
    if (vfs_create(name) == 0) {
      vga_print("File created\n");
      serial_print("File created\n");
    } else {
      vga_print("Create failed\n");
      serial_print("Create failed\n");
    }
  } else if (cmd[0] == 'r' && cmd[1] == 'm' && cmd[2] == ' ') {
    const char *name = &cmd[3];
    if (vfs_delete(name) == 0) {
      vga_print("File removed\n");
      serial_print("File removed\n");
    } else {
      vga_print("Remove failed\n");
      serial_print("Remove failed\n");
    }
  } else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ') {
    const char *name = &cmd[4];
    char buf[256];
    if (vfs_read(name, buf) == 0) {
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
      if (vfs_write(name, data) == 0) {
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
             cmd[8] == 'e' && cmd[9] == 'a' && cmd[10] == 'd') {
    /* Konfirmasi sebelum mulai demo */
    vga_print("\nDemo 2 Kernel Thread (transisi 5 status)\n");
    vga_print("Tekan ENTER untuk mulai, ketik 'q' lalu ENTER untuk batal: ");
    serial_print("\nDemo 2 Kernel Thread (transisi 5 status)\n");
    serial_print("Tekan ENTER untuk mulai, ketik 'q' lalu ENTER untuk batal: ");
    {
      char confirm[8];
      int ci = 0;
      while (1) {
        char cc = input_getchar();
        if (cc == '\r' || cc == '\n') {
          confirm[ci] = '\0';
          vga_putchar('\n');
          serial_putchar('\n');
          break;
        } else if (cc == '\b' && ci > 0) {
          ci--;
          vga_putchar('\b');
          serial_print("\b \b");
        } else if (ci < 6 && cc >= 32 && cc <= 126) {
          confirm[ci++] = cc;
          vga_putchar(cc);
          serial_putchar(cc);
        }
      }
      if (confirm[0] == 'q' || confirm[0] == 'Q') {
        vga_print("Demo dibatalkan.\n");
        serial_print("Demo dibatalkan.\n");
      } else {
        task_demo_run();
        /* Tunggu user menekan ENTER untuk kembali ke shell */
        vga_print("Tekan ENTER untuk kembali ke shell...");
        serial_print("Tekan ENTER untuk kembali ke shell...");
        while (1) {
          char wc = input_getchar();
          if (wc == '\r' || wc == '\n') {
            vga_putchar('\n');
            serial_putchar('\n');
            break;
          }
        }
      }
    }
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
