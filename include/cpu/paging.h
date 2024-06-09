#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

typedef struct page_struct_t {
    uint32_t page_directory[1024] __attribute__((aligned(4096)));
    uint32_t *page_tables[1024] __attribute__((aligned(4096)));
    unsigned char *bitmap;
} PAGE_STRUCT;

void enable_paging();
int get_first_physical_page();
void* mark_physical_page_used(void * address);
void* mark_physical_page_free(void * address);
void switch_cr3(void * new_cr3);
PAGE_STRUCT* copy_nonkernel_pages(PAGE_STRUCT* old);

void map_page(PAGE_STRUCT* pages, uint32_t virtual, uint32_t physical);
void free_page(PAGE_STRUCT* pages, uint32_t virtual);


PAGE_STRUCT kernel_pages;

#endif