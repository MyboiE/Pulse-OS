#ifndef PROTECT_H
#define PROTECT_H

typedef enum {
    ROLE_VISITOR_ADMIN = 0,
    ROLE_EMERGENCY_RECEPTION = 1
} user_role_t;

#define ROLE_GUEST ROLE_VISITOR_ADMIN
#define ROLE_USER ROLE_VISITOR_ADMIN
#define ROLE_ADMIN ROLE_EMERGENCY_RECEPTION

typedef enum {
    RESOURCE_VISITOR_ADMIN = 0,
    RESOURCE_EMERGENCY = 1
} protected_resource_t;

void protect_init(void);
int check_access_permission(user_role_t current_role, user_role_t required_role);
int protect_resource_allowed(protected_resource_t resource);
int protect_set_role(user_role_t role);
const char *protect_role_name(void);
void test_memory_protection_violation(void);
void protect_self_test(void);
int protect_current_role(void);
int protect_authorize(user_role_t required_role);

#endif