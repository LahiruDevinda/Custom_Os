#ifndef PROCESS_H
#define PROCESS_H

#include "../include/types.h"

#define MAX_PROCESSES   16
#define STACK_SIZE      4096

/* Process States from L09 Lecture Notes */
typedef enum {
    PROC_READY   = 0,
    PROC_RUNNING = 1,
    PROC_BLOCKED = 2,
    PROC_ZOMBIE  = 3
} proc_state_t;

/* Process Control Block (PCB) */
typedef struct pcb {
    int          pid;
    proc_state_t state;
    uint32_t     esp;                  /* Saved kernel stack pointer */
    uint32_t     eip;                  /* Instruction pointer */
    int          priority;
    char         name[32];
    uint8_t      kstack[STACK_SIZE];   /* 4KB dedicated process stack */
} pcb_t;

/* External assembly context switcher */
extern void context_switch(uint32_t *old_esp, uint32_t new_esp);

/* Core Process Management APIs */
void   process_init(void);
int    create_process(const char *name, void (*entry)(void), int priority);
void   process_yield(void);
void   process_exit(void);
void   cmd_ps(void);
void   cmd_kill(const char *arg);

#endif /* PROCESS_H */