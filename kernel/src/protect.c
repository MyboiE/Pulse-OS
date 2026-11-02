#include "../include/kernel.h"
#include "../include/protect.h"

static user_role_t current_system_role = ROLE_GUEST;

void protect_init(void) {
    current_system_role = ROLE_ADMIN; // Default mode kernel
}

int check_access_permission(user_role_t current_role, user_role_t required_role) {
    if (current_role >= required_role) {
        return 1; // Akses Diterima
    }
    return 0; // Akses Ditolak
}

void test_memory_protection_violation(void) {
    vga_print("\n[PROTECT] Menguji akses ruang memori terlarang...\n");
    if (!check_access_permission(ROLE_GUEST, ROLE_ADMIN)) {
        vga_print("[PROTECT] AKSES DITOLAK: User Mode dilarang mengakses Kernel Space!\n");
    }
}