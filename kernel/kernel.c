/* =============================================================================
 * SENG21213-OS :: Main Kernel  (Stage 0 – Foundations)
 * File   : kernel/kernel.c
 *
 * PURPOSE
 *   This is the heart of your operating system. Right now it:
 *     1. Initialises VGA text-mode display
 *     2. Initialises the keyboard driver
 *     3. Prints a splash screen
 *     4. Runs a minimal interactive shell ("ksh")
 *
 * ASSIGNMENT MILESTONES  (what YOU will add in later lectures)
 *   Lecture  9  – Process Management  →  process.h / process.c / scheduler.c
 *   Lecture 10  – Threads             →  thread.h  / thread.c
 *   Lecture 11  – Memory Management   →  pmm.h     / pmm.c / vmm.c
 *   Lecture 12  – File System         →  fs.h      / fs.c
 *
 * CODING CONVENTION
 *   - Prefix kernel-internal functions with k_ (e.g. k_strcmp)
 *   - All driver APIs live in their own .h/.c pair
 *   - NEVER call malloc – use the PMM you build in Lecture 11
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "../include/types.h"

/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_greet(const char *args);

/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers (no libc in a freestanding kernel!)
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

/* Skip leading spaces */
static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/
static void print_splash(void) {
    vga_clear(VGA_BLACK);

    /* Outer border frame */
    vga_draw_box(0, 0, 8, 80, VGA_CYAN);

    /* ASCII Header Logo */
    vga_set_cursor(1, 4);
    vga_puts_color("  ____  _____ _   _  ____ ____  _ ____  _ _____        ___  ____ ",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_set_cursor(2, 4);
    vga_puts_color(" / ___|| ____| \\ | |/ ___|___ \\/ |___ \\/ |___ /       / _ \\/ ___|",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_set_cursor(3, 4);
    vga_puts_color(" \\___ \\|  _| |  \\| | |  _  __) | | __) | | |_ \\ _____| | | \\___ \\",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_set_cursor(4, 4);
    vga_puts_color("  ___) | |___| |\\  | |_| |/ __/| |/ __/| |___) |_____| |_| |___) |",
                   VGA_WHITE, VGA_BLACK);
    vga_set_cursor(5, 4);
    vga_puts_color(" |____/|_____|_| \\_|\\____|_____|_|_____|_|____/       \\___/|____/ ",
                   VGA_WHITE, VGA_BLACK);               

    vga_set_cursor(6, 12);
    vga_puts_color("x86 Protected Mode Kernel  ::  Stage 0 Foundations",
                   VGA_LIGHT_MAGENTA, VGA_BLACK);
    /* My Profile Box */
    vga_set_cursor(9, 2);
    vga_puts_color("Hello, ", VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(10, 4);
    vga_puts_color("I am Lahiru Devinda", VGA_YELLOW, VGA_BLACK);

    vga_set_cursor(11, 4);
    vga_puts_color("Index Number  : ", VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts_color("SE/2023/041", VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(12, 4);
    vga_puts_color("Department    : ", VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts_color("Software Engineering Teaching Unit | Faculty of Science", VGA_LIGHT_GREY, VGA_BLACK);

    /* Hardware & Kernel Environment Badges */
    vga_set_cursor(14, 2);
    vga_puts_color("[ BOOT ENVIRONMENT ]", VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(15, 4);
    vga_puts_color("CPU Mode  : ", VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts_color("32-bit x86 Protected Mode (i686)", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(16, 4);
    vga_puts_color("Display   : ", VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts_color("VGA 80x25 Color Text Mode Buffer [0xB8000]", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(17, 4);
    vga_puts_color("Keyboard  : ", VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts_color("PS/2 Polling Controller Ready", VGA_LIGHT_GREEN, VGA_BLACK);

    /* Interactive Instruction at Bottom */
    
    vga_set_cursor(20, 0);
    vga_puts_color("  System initialized successfully. Type ", VGA_WHITE, VGA_BLACK);
    vga_puts_color("'help'", VGA_YELLOW, VGA_BLACK);
    vga_puts_color(" to explore available commands.", VGA_WHITE, VGA_BLACK);

    vga_set_cursor(22, 0);
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ------------------------------------------------\n");
    vga_puts("  help    - Show this help message\n");
    vga_puts("  clear   - Clear the screen\n");
    vga_puts("  about   - About this OS and course\n");
    vga_puts("  echo    - Echo text to screen\n");
    vga_puts("  mem     - Memory map (stub)\n");
    vga_puts_color("\n  Milestones (to implement):\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ps      - [L09] List processes\n");
    vga_puts("  kill    - [L09] Terminate a process\n");
    vga_puts("  threads - [L10] List kernel threads\n");
    vga_puts("  free    - [L11] Show free memory\n");
    vga_puts("  ls      - [L12] List files\n");
    vga_puts("  cat     - [L12] Print file contents\n\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_about(void) {
    vga_puts_color("\n  About SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 - Sem 2\n");
    vga_puts("  Reference    : Stallings, OS: Internals & Design Principles\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_mem(void) {
    /* Stage 0 stub – students implement the real PMM in Lecture 11 */
    vga_puts_color("\n  Memory Map (stub - implement PMM in Lecture 11)\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");
    vga_puts("  0x00000000 - 0x000FFFFF  :  First 1 MB (reserved/BIOS)\n");
    vga_puts("  0x00100000 - 0x00EFFFFF  :  Extended memory (usable ~14 MB)\n");
    vga_puts("  0x00F00000 - 0x00FFFFFF  :  BIOS / ROM area\n");
    vga_puts("  0xB8000    - 0xBFFFF     :  VGA frame buffer\n");
    vga_puts_color("\n  TODO: Use BIOS int 0x15, EAX=0xE820 to get real memory map\n\n",
                   VGA_YELLOW, VGA_BLACK);
}

//print greeting message
static void cmd_greet(const char *args) {

    vga_puts_color("Hello, ",VGA_YELLOW, VGA_BLACK);
    vga_puts_color(args, VGA_LIGHT_CYAN, VGA_BLACK);
    
}

/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/
static char  shell_buf[256];
static char  prompt[] = "\n  ksh> ";

static void shell_run(void) {

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        /* Trim leading whitespace */
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        /* Dispatch */
        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "about") == 0) { cmd_about(); continue; }
        if (k_strcmp(cmd, "mem")   == 0) { cmd_mem();   continue; }

        if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }
         if (k_strncmp(cmd, "greet ", 5) == 0) {
            cmd_greet(k_ltrim(cmd + 5));
            continue;
        }

        /* Milestone stubs */
        if (k_strcmp(cmd, "ps")      == 0 ||
            k_strcmp(cmd, "kill")    == 0 ||
            k_strcmp(cmd, "threads") == 0 ||
            k_strcmp(cmd, "free")    == 0 ||
            k_strcmp(cmd, "ls")      == 0 ||
            k_strcmp(cmd, "cat")     == 0) {
            vga_puts_color("  [TODO] This command is not yet implemented.\n",
                           VGA_YELLOW, VGA_BLACK);
            vga_puts("  Implement it as part of your lecture assignment.\n");
            continue;
        }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

/* ---------------------------------------------------------------------------
 * Kernel entry point – called from kernel_entry.asm
 * --------------------------------------------------------------------------*/
void kernel_main(void) {
    vga_init();
    kb_init();
    print_splash();
    shell_run();

    /* Should never reach here */
    __asm__ __volatile__("hlt");
}
