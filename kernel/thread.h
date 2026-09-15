#ifndef THREAD_H
#define THREAD_H

#include "../include/types.h"

#define MAX_THREADS 16
#define THREAD_STACK_SIZE 2048

typedef enum {
    THREAD_READY = 0,
    THREAD_RUNNING = 1,
    THREAD_BLOCKED = 2,
    THREAD_TERMINATED = 3
} thread_state_t;

/* Thread Control Block (TCB) */
typedef struct tcb {
    int            tid;
    int            owner_pid;
    thread_state_t state;
    uint32_t       esp;
    char           name[32];
    uint8_t        stack[THREAD_STACK_SIZE];
} tcb_t;

/* Mutex Structure */
typedef struct {
    volatile int locked;
    int          owner_tid;
} mutex_t;

/* Function Declarations */
void thread_init(void);
int  thread_create(const char *name, void (*entry)(void), int owner_pid);
void thread_yield(void);
void thread_exit(void);
void cmd_threads(void);

/* Mutex primitives */
void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

#endif /* THREAD_H */
