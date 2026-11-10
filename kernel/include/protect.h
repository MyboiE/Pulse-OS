#ifndef PROTECT_H
#define PROTECT_H

typedef enum {
    ROLE_GUEST = 0,
    ROLE_USER = 1,
    ROLE_ADMIN = 2
} user_role_t;

void protect_init(void);
int check_access_permission(user_role_t current_role, user_role_t required_role);
void test_memory_protection_violation(void);
void protect_self_test(void);
int protect_current_role(void);
int protect_authorize(user_role_t required_role);

#endif