#include "pmm.h"
#include "vga.h"

/* Bitmap: 1 bit = 1 page frame (1 = used/reserved, 0 = free) */
static uint8_t  pmm_bitmap[BITMAP_SIZE];
static uint32_t free_pages_count = 0;

static inline void bitmap_set(uint32_t bit) {
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(uint32_t bit) {
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline int bitmap_test(uint32_t bit) {
    return (pmm_bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

void pmm_init(uint32_t kernel_end) {
    /* 1. Mark all memory as used initially */
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0xFF;
    }
    free_pages_count = 0;

    /* 2. Free pages above kernel space up to TOTAL_MEMORY */
    /* Align kernel end to next 4KB boundary */
    uint32_t start_page = (kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t page = start_page; page < TOTAL_PAGES; page++) {
        bitmap_clear(page);
        free_pages_count++;
    }
}

uint32_t pmm_alloc_page(void) {
    for (uint32_t i = 0; i < TOTAL_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            free_pages_count--;
            return i * PAGE_SIZE;
        }
    }
    return 0; /* Out of memory */
}

void pmm_free_page(uint32_t page_addr) {
    uint32_t page = page_addr / PAGE_SIZE;
    if (page < TOTAL_PAGES && bitmap_test(page)) {
        bitmap_clear(page);
        free_pages_count++;
    }
}

uint32_t pmm_get_free_pages(void) {
    return free_pages_count;
}

uint32_t pmm_get_total_pages(void) {
    return TOTAL_PAGES;
}

void cmd_free(void) {
    uint32_t total = TOTAL_PAGES;
    uint32_t free_p = free_pages_count;
    uint32_t used_p = total - free_p;

    uint32_t total_kb = (total * PAGE_SIZE) / 1024;
    uint32_t free_kb  = (free_p * PAGE_SIZE) / 1024;
    uint32_t used_kb  = (used_p * PAGE_SIZE) / 1024;

    vga_puts_color("\n  Physical Memory Management Statistics\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");
    vga_printf("  Total Memory : %u KB (%u pages)\n", total_kb, total);
    vga_printf("  Used Memory  : %u KB (%u pages)\n", used_kb, used_p);
    vga_printf("  Free Memory  : %u KB (%u pages)\n", free_kb, free_p);
    vga_printf("  Page Size    : %u bytes\n\n", PAGE_SIZE);
}
