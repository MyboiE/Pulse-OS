#include "../include/kernel.h"
#include "../include/protect.h"

static user_role_t current_system_role = ROLE_GUEST;

void protect_init(void) {
    current_system_role = ROLE_ADMIN;
}

int check_access_permission(user_role_t current_role, user_role_t required_role) {
    if (current_role < ROLE_GUEST || current_role > ROLE_ADMIN ||
        required_role < ROLE_GUEST || required_role > ROLE_ADMIN) return 0;
    return current_role >= required_role;
}

static void test_result(int ok, const char* message) {
    const char* result = ok ? "[PASS] " : "[FAIL] ";
    vga_print(result);
    vga_print(message);
    vga_print("\n");
    serial_print(result);
    serial_print(message);
    serial_print("\n");
}

void test_memory_protection_violation(void) {
    vga_print("\n[PROTECT] Role access-control test (not hardware memory isolation)\n");
    test_result(check_access_permission(ROLE_ADMIN, ROLE_USER),
                "admin allowed for user-level resource");
    test_result(!check_access_permission(ROLE_GUEST, ROLE_ADMIN),
                "guest denied for admin-level resource");
}

void protect_self_test(void) {
    test_memory_protection_violation();
}

int protect_current_role(void) {
    return current_system_role;
}

int protect_authorize(user_role_t required_role) {
    if (!check_access_permission(current_system_role, required_role)) {
        vga_print("\n[DENIED] Izin akses tidak cukup.\n");
        serial_print("[DENIED] Insufficient access permission.\n");
        return 0;
    }
    return 1;
}
