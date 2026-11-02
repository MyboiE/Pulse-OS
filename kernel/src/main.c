#include "../include/kernel.h"
#include "../include/cli.h"
#include "../include/task.h"
#include "../include/memory.h"
#include "../include/vfs.h"
#include "../include/sync.h"
#include "../include/protect.h"

void kmain(void) {
    // Inisialisasi Driver Display & Serial & Keyboard
    vga_clear();
    serial_init();
    keyboard_init();

    // Inisialisasi Subsystem Kernel
    memory_init();
    vfs_init();
    task_sys_init();
    protect_init();

    // Cetak Banner & Logo ASCII Booting
    cli_print_logo();
    vga_print("Pulse-OS Kernel Berhasil Di-boot!\n");
    serial_print("[KERNEL] Pulse-OS booted successfully.\n");

    // Jalankan Shell CLI Internal
    cli_run();
}

void sys_restart(void) {
    vga_print("\nMerestart Pulse-OS...\n");
    serial_print("\n[KERNEL] Restarting Pulse-OS...\n");

    // 1. Keyboard Controller (8042) reset line
    unsigned char status;
    do {
        status = inb(0x64);
        if (status & 1) (void)inb(0x60);
    } while (status & 2);
    outb(0x64, 0xFE);

    // 2. PCI Reset Register (Port 0xCF9)
    outb(0xCF9, 0x02);
    outb(0xCF9, 0x06);

    // 3. Fallback: Triple fault (selalu memaksa QEMU hard-reset)
    struct { unsigned short limit; unsigned int base; } __attribute__((packed)) null_idt = {0, 0};
    __asm__ volatile ("lidt %0" : : "m"(null_idt));
    __asm__ volatile ("int $3");

    __asm__ volatile ("cli");
    while (1) { __asm__ volatile ("hlt"); }
}

void sys_shutdown(void) {
    vga_print("\nMematikan Pulse-OS...\n");
    serial_print("\n[KERNEL] Shutting down Pulse-OS...\n");

    // QEMU ACPI shutdown
    outw(0x604, 0x2000);

    // Bochs / older QEMU
    outw(0xB004, 0x2000);

    // VirtualBox
    outw(0x4004, 0x3400);

    // Fallback: halt CPU jika hypervisor tidak exit otomatis atau berjalan di bare-metal
    vga_print("Sistem dihentikan. Komputer dapat dimatikan dengan aman.\n");
    serial_print("[KERNEL] System halted.\n");
    __asm__ volatile ("cli");
    while (1) {
        __asm__ volatile ("hlt");
    }
}