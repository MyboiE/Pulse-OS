#ifndef TASK_H
#define TASK_H

typedef enum {
    THREAD_NEW,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_WAITING,
    THREAD_TERMINATED
} thread_state_t;

typedef struct thread {
    int id;
    char name[32];
    thread_state_t state;
    void (*entry)(void);
    unsigned int stack_pointer;
    struct thread* next;
} thread_t;

void task_sys_init(void);
thread_t* thread_create(const char* name, void (*entry)(void));
void thread_yield(void);
void schedule(void);
void print_thread_states(void);

#endif