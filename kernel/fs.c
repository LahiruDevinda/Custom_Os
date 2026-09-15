#include "fs.h"
#include "vga.h"

static file_entry_t file_table[MAX_FILES];

static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static void k_strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static size_t k_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int fs_create(const char *name, const char *content) {
    int slot = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!file_table[i].used) {
            slot = i;
            break;
        }
    }

    if (slot == -1) return -1;

    k_strncpy(file_table[slot].name, name, MAX_FILENAME);
    size_t len = k_strlen(content);
    if (len >= MAX_FILE_SIZE) len = MAX_FILE_SIZE - 1;

    for (size_t i = 0; i < len; i++) {
        file_table[slot].data[i] = (uint8_t)content[i];
    }
    file_table[slot].data[len] = '\0';
    file_table[slot].size = len;
    file_table[slot].used = true;

    return 0;
}

void fs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        file_table[i].used = false;
        file_table[i].size = 0;
    }

    /* Seed initial demo files */
    fs_create("readme.txt", "SENG21213 Operating System\nBare-metal x86 Protected Mode Kernel.\n");
    fs_create("authors.txt", "Developer: Lahiru Devinda\nIndex: SE/2023/041\n");
    fs_create("version.sys", "Kernel v1.0.4-stage4\nBuild date: 2026\n");
}

void cmd_ls(void) {
    vga_puts_color("\n  NAME                 SIZE (bytes)\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ----------------------------------------\n");

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used) {
            vga_puts("  ");
            vga_puts(file_table[i].name);

            /* Pad with spaces up to 21 columns */
            int len = (int)k_strlen(file_table[i].name);
            while (len < 21) {
                vga_putchar(' ');
                len++;
            }

            vga_printf("%u\n", file_table[i].size);
        }
    }
    vga_puts("\n");
}

void cmd_cat(const char *filename) {
    while (*filename == ' ') filename++;

    if (*filename == '\0') {
        vga_puts_color("  Usage: cat <filename>\n", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && k_strcmp(file_table[i].name, filename) == 0) {
            vga_puts("\n");
            vga_puts((const char *)file_table[i].data);
            if (file_table[i].data[file_table[i].size - 1] != '\n') {
                vga_puts("\n");
            }
            vga_puts("\n");
            return;
        }
    }

    vga_puts_color("  cat: file not found: ", VGA_LIGHT_RED, VGA_BLACK);
    vga_puts(filename);
    vga_puts("\n");
}
