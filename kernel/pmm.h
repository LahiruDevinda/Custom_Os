#ifndef PMM_H
#define PMM_H

#include "../include/types.h"

#define PAGE_SIZE       4096
#define TOTAL_MEMORY    (16 * 1024 * 1024)   /* Manage 16 MB of RAM */
#define TOTAL_PAGES     (TOTAL_MEMORY / PAGE_SIZE)
#define BITMAP_SIZE     (TOTAL_PAGES / 8)

/* Core PMM APIs */
void     pmm_init(uint32_t kernel_end);
uint32_t pmm_alloc_page(void);
void     pmm_free_page(uint32_t page_addr);
uint32_t pmm_get_free_pages(void);
uint32_t pmm_get_total_pages(void);
void     cmd_free(void);

#endif /* PMM_H */
