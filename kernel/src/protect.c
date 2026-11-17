#include "../include/kernel.h"
#include "../include/protect.h"
#include "../include/vfs.h"

static user_role_t current_system_role = ROLE_EMERGENCY_RECEPTION;

void protect_init(void) {
    current_system_role = ROLE_EMERGENCY_RECEPTION;
}

int check_access_permission(user_role_t current_role, user_role_t required_role) {
    if ((current_role != ROLE_VISITOR_ADMIN && current_role != ROLE_EMERGENCY_RECEPTION) ||
        (required_role != ROLE_VISITOR_ADMIN && required_role != ROLE_EMERGENCY_RECEPTION)) return 0;
    return current_role >= required_role;
}

int protect_resource_allowed(protected_resource_t resource) {
    if (resource == RESOURCE_VISITOR_ADMIN)
        return current_system_role == ROLE_VISITOR_ADMIN || current_system_role == ROLE_EMERGENCY_RECEPTION;
    if (resource == RESOURCE_EMERGENCY)
        return current_system_role == ROLE_EMERGENCY_RECEPTION;
    return 0;
}

int protect_set_role(user_role_t role) {
    if (role != ROLE_VISITOR_ADMIN && role != ROLE_EMERGENCY_RECEPTION) return 0;
    current_system_role = role;
    return 1;
}

const char *protect_role_name(void) {
    return current_system_role == ROLE_EMERGENCY_RECEPTION ? "Emergency Reception" : "Visitor Administration";
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
    vga_print("\n[PROTECT] Role access-control test (software RBAC; not hardware memory isolation)\n");
    test_result(check_access_permission(ROLE_EMERGENCY_RECEPTION, ROLE_VISITOR_ADMIN),
                "emergency reception allowed for visitor administration");
    test_result(!check_access_permission(ROLE_VISITOR_ADMIN, ROLE_EMERGENCY_RECEPTION),
                "visitor administration denied emergency access");
    test_result(!check_access_permission((user_role_t)99, ROLE_VISITOR_ADMIN),
                "invalid role rejected");
}

void protect_self_test(void) {
    user_role_t saved_role = current_system_role;
    protect_set_role(ROLE_VISITOR_ADMIN);
    test_result(protect_resource_allowed(RESOURCE_VISITOR_ADMIN),
                "visitor administration operation allowed");
    test_result(!protect_resource_allowed(RESOURCE_EMERGENCY),
                "visitor administration emergency operation denied");
    protect_set_role(ROLE_EMERGENCY_RECEPTION);
    test_result(protect_resource_allowed(RESOURCE_EMERGENCY),
                "emergency reception operation allowed");

    int emergency_ready = vfs_create("rbac-emergency.test") == 0 &&
                          vfs_write("rbac-emergency.test", "protected") == 0;
    protect_set_role(ROLE_VISITOR_ADMIN);
    int visitor_create = vfs_create("visitor-rbac.test") == 0;
    int visitor_write = visitor_create && vfs_write("visitor-rbac.test", "visitor") == 0;
    char data[32];
    int visitor_read = visitor_write && vfs_read("visitor-rbac.test", data, sizeof(data)) == 0;
    int visitor_delete = visitor_read && vfs_delete("visitor-rbac.test") == 0;
    test_result(visitor_create && visitor_write && visitor_read && visitor_delete,
                "VFS visitor create/read/write/delete allowed");
    test_result(emergency_ready && vfs_read("rbac-emergency.test", data, sizeof(data)) != 0,
                "VFS visitor read emergency denied");
    test_result(emergency_ready && vfs_write("rbac-emergency.test", "tampered") != 0,
                "VFS visitor write emergency denied");
    test_result(vfs_create("rbac-denied.test") != 0,
                "VFS visitor create emergency denied");
    test_result(emergency_ready && vfs_delete("rbac-emergency.test") != 0,
                "VFS visitor delete emergency denied");

    protect_set_role(ROLE_EMERGENCY_RECEPTION);
    int emergency_access = emergency_ready &&
        vfs_read("rbac-emergency.test", data, sizeof(data)) == 0 &&
        vfs_write("rbac-emergency.test", "authorized") == 0 &&
        vfs_delete("rbac-emergency.test") == 0;
    test_result(emergency_access, "VFS emergency reception read/write/delete allowed");
    protect_set_role(saved_role);
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
