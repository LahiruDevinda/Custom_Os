#include "process.h"
#include "vga.h"

static pcb_t process_table[MAX_PROCESSES];
static int   current_pid = 0;
static int   next_pid = 1;

/* String helpers */
static void k_strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int k_atoi(const char *s) {
    int res = 0;
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}

/* Initialize PCB table and slot 0 for main kernel shell */
void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = -1;
        process_table[i].state = PROC_ZOMBIE;
    }

    /* PID 0: The running kernel shell */
    process_table[0].pid = 0;
    process_table[0].state = PROC_RUNNING;
    process_table[0].priority = 1;
    k_strncpy(process_table[0].name, "kernel_shell", 32);
    current_pid = 0;
}

/* Allocate stack, setup initial register frame, and mark READY */
int create_process(const char *name, void (*entry)(void), int priority) {
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == -1 || process_table[i].state == PROC_ZOMBIE) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        vga_puts_color("  [Error] Process table full!\n", VGA_LIGHT_RED, VGA_BLACK);
        return -1;
    }

    pcb_t *p = &process_table[slot];
    p->pid = next_pid++;
    p->priority = priority;
    p->state = PROC_READY;
    p->eip = (uint32_t)entry;
    k_strncpy(p->name, name, 32);

    /* Setup stack frame (grows downward) */
    uint32_t *stk = (uint32_t *)(p->kstack + STACK_SIZE);

    /* 1. Return address when switch_to executes ret */
    *(--stk) = (uint32_t)entry;

    /* 2. EFLAGS (interrupts enabled = 0x202) */
    *(--stk) = 0x00000202;

    /* 3. General-purpose registers pushed by pushad */
    *(--stk) = 0; /* EAX */
    *(--stk) = 0; /* ECX */
    *(--stk) = 0; /* EDX */
    *(--stk) = 0; /* EBX */
    *(--stk) = 0; /* ESP (dummy) */
    *(--stk) = 0; /* EBP */
    *(--stk) = 0; /* ESI */
    *(--stk) = 0; /* EDI */

    p->esp = (uint32_t)stk;
    return p->pid;
}

/* Round-Robin Scheduler */
void process_yield(void) {
    int next_slot = -1;
    int search = (current_pid + 1) % MAX_PROCESSES;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        int idx = (search + i) % MAX_PROCESSES;
        if (process_table[idx].pid != -1 && process_table[idx].state == PROC_READY) {
            next_slot = idx;
            break;
        }
    }

    if (next_slot == -1 || next_slot == current_pid) {
        return; /* No other ready process */
    }

    int prev_slot = current_pid;
    if (process_table[prev_slot].state == PROC_RUNNING) {
        process_table[prev_slot].state = PROC_READY;
    }

    process_table[next_slot].state = PROC_RUNNING;
    current_pid = next_slot;

    /* Switch stacks and execute */
    context_switch(&process_table[prev_slot].esp, process_table[next_slot].esp);
}

/* Terminate current process */
void process_exit(void) {
    process_table[current_pid].state = PROC_ZOMBIE;
    process_yield();
    while (1) { __asm__ __volatile__("hlt"); }
}

/* Helper to print a string with space padding up to 'width' characters */
static void print_padded(const char *s, int width) {
    int len = 0;
    while (s[len]) {
        vga_putchar(s[len]);
        len++;
    }
    while (len < width) {
        vga_putchar(' ');
        len++;
    }
}

/* 'ps' shell command: List all active processes */
void cmd_ps(void) {
    vga_puts_color("\n  PID   NAME            STATE       PRIORITY\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");

    const char *state_str[] = {"READY", "RUNNING", "BLOCKED", "ZOMBIE"};

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid != -1) {
            vga_printf("  %d", process_table[i].pid);
            if (process_table[i].pid < 10) vga_puts("     ");
            else vga_puts("    ");

            print_padded(process_table[i].name, 16);
            print_padded(state_str[process_table[i].state], 12);
            vga_printf("%d\n", process_table[i].priority);
        }
    }
    vga_puts("\n");
}

/* 'kill <pid>' shell command */
void cmd_kill(const char *arg) {
    while (*arg == ' ') arg++;
    int pid = k_atoi(arg);

    if (pid == 0) {
        vga_puts_color("  Cannot terminate root kernel shell (PID 0)!\n", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROC_ZOMBIE) {
            process_table[i].state = PROC_ZOMBIE;
            vga_printf("  Process [%d] terminated.\n", pid);
            return;
        }
    }
    vga_printf("  Process [%d] not found.\n", pid);
}
