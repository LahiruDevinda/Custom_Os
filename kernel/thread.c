#include "thread.h"
#include "process.h"
#include "vga.h"

static tcb_t thread_table[MAX_THREADS];
static int   current_tid = 0;
static int   next_tid = 1;

static void k_strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

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

void thread_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_table[i].tid = -1;
        thread_table[i].state = THREAD_TERMINATED;
    }

    /* Thread 0 represents the main kernel shell thread */
    thread_table[0].tid = 0;
    thread_table[0].owner_pid = 0;
    thread_table[0].state = THREAD_RUNNING;
    k_strncpy(thread_table[0].name, "shell_main", 32);
    current_tid = 0;
}

int thread_create(const char *name, void (*entry)(void), int owner_pid) {
    int slot = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].tid == -1 || thread_table[i].state == THREAD_TERMINATED) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        vga_puts_color("  [Error] Thread limit reached!\n", VGA_LIGHT_RED, VGA_BLACK);
        return -1;
    }

    tcb_t *t = &thread_table[slot];
    t->tid = next_tid++;
    t->owner_pid = owner_pid;
    t->state = THREAD_READY;
    k_strncpy(t->name, name, 32);

    /* Setup thread stack */
    uint32_t *stk = (uint32_t *)(t->stack + THREAD_STACK_SIZE);

    *(--stk) = (uint32_t)entry;
    *(--stk) = 0x00000202; /* EFLAGS */
    *(--stk) = 0;          /* EAX */
    *(--stk) = 0;          /* ECX */
    *(--stk) = 0;          /* EDX */
    *(--stk) = 0;          /* EBX */
    *(--stk) = 0;          /* ESP */
    *(--stk) = 0;          /* EBP */
    *(--stk) = 0;          /* ESI */
    *(--stk) = 0;          /* EDI */

    t->esp = (uint32_t)stk;
    return t->tid;
}

void thread_yield(void) {
    int next_slot = -1;
    int search = (current_tid + 1) % MAX_THREADS;

    for (int i = 0; i < MAX_THREADS; i++) {
        int idx = (search + i) % MAX_THREADS;
        if (thread_table[idx].tid != -1 && thread_table[idx].state == THREAD_READY) {
            next_slot = idx;
            break;
        }
    }

    if (next_slot == -1 || next_slot == current_tid) {
        return;
    }

    int prev_slot = current_tid;
    if (thread_table[prev_slot].state == THREAD_RUNNING) {
        thread_table[prev_slot].state = THREAD_READY;
    }

    thread_table[next_slot].state = THREAD_RUNNING;
    current_tid = next_slot;

    context_switch(&thread_table[prev_slot].esp, thread_table[next_slot].esp);
}

void thread_exit(void) {
    thread_table[current_tid].state = THREAD_TERMINATED;
    thread_yield();
    while (1) { __asm__ __volatile__("hlt"); }
}

/* Mutex Synchronization Primitives */
void mutex_init(mutex_t *m) {
    m->locked = 0;
    m->owner_tid = -1;
}

void mutex_lock(mutex_t *m) {
    while (__sync_lock_test_and_set(&m->locked, 1)) {
        thread_yield();
    }
    m->owner_tid = current_tid;
}

void mutex_unlock(mutex_t *m) {
    if (m->owner_tid == current_tid) {
        m->owner_tid = -1;
        __sync_lock_release(&m->locked);
    }
}

/* 'threads' command: display all threads in table */
void cmd_threads(void) {
    vga_puts_color("\n  TID   PID   NAME            STATE\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");

    const char *state_str[] = {"READY", "RUNNING", "BLOCKED", "TERMINATED"};

    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].tid != -1) {
            vga_printf("  %d", thread_table[i].tid);
            if (thread_table[i].tid < 10) vga_puts("     ");
            else vga_puts("    ");

            vga_printf("%d", thread_table[i].owner_pid);
            if (thread_table[i].owner_pid < 10) vga_puts("     ");
            else vga_puts("    ");

            print_padded(thread_table[i].name, 16);
            vga_puts(state_str[thread_table[i].state]);
            vga_puts("\n");
        }
    }
    vga_puts("\n");
}
