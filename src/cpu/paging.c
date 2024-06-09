/**
 * @defgroup   PAGING paging
 * @ingroup    CPU 
 *
 * @brief      This file implements paging.
 * @todo       Rewrite
 *
 * @author     Valerie Whitmire
 * @date       2023
 */
#include "libc/mem.h"
#include "libc/bitmap.h"
#include "drivers/screen.h"
#include "libc/string.h"
#include "cpu/isr.h"
#include "cpu/paging.h"
#include "libc/function.h"

uint32_t kernel_page_directory[1024] __attribute__((aligned(4096)));

PAGE_STRUCT kernel_pages;
unsigned char *physical_page_bitmap;

void page_fault() {
    kprintn("oops");
    while(1);
}

void enable_paging() {
    kernel_pages.bitmap = ta_alloc(sizeof(char) * 0x100400);
    physical_page_bitmap = ta_alloc(sizeof(char) * 0x100400);
    int i;
    for(i = 0; i < 1024; i++) {
        kernel_pages.page_tables[i] = ta_alloc_align(sizeof(uint32_t)*1024, 4096);
        int j = 0;
        for(j = 0; j < 1024; j++) {
            if((i*1024+j) * 0x1000 < 0x4fff000 || (i*1024+j) * 0x1000 > 0x7000000) {
                kernel_pages.page_tables[i][j] = ((i*1024+j) * 0x1000) | 3; // attributes: supervisor level, read/write, present.
                bitmapSet(physical_page_bitmap, i*1024+j);
                bitmapSet(kernel_pages.bitmap, i*1024+j);
            } else {
                kernel_pages.page_tables[i][j] = 0b000;
                bitmapReset(physical_page_bitmap, i*1024+j);
                bitmapReset(kernel_pages.bitmap, i*1024+j);
            }
        }
        kernel_pages.page_directory[i] = ((unsigned int)kernel_pages.page_tables[i]) | 3;
    }

    switch_cr3(&kernel_pages.page_directory);

    register_interrupt_handler(14, page_fault);
}


PAGE_STRUCT* copy_nonkernel_pages(PAGE_STRUCT* old) {
    PAGE_STRUCT* new = ta_alloc_align(sizeof(PAGE_STRUCT), 4096);
    new->bitmap = ta_alloc(sizeof(char) * 0x100400);  
    int i;
    for(i = 0; i < 1024; i++) {
        new->page_tables[i] = ta_alloc_align(sizeof(uint32_t)*1024, 4096);
        int j = 0;
        for(j = 0; j < 1024; j++) {
            if((i*1024+j) * 0x1000 < 0x4fff000 || (i*1024+j) * 0x1000 > 0x7000000) {
                new->page_tables[i][j] = ((i*1024+j) * 0x1000) | 3; // attributes: supervisor level, read/write, present.
                bitmapSet(new->bitmap, i*1024+j);
            } else {
                new->page_tables[i][j] = 0b000;
                bitmapReset(new->bitmap, i*1024+j);
            }
        }
        new->page_directory[i] = ((unsigned int)new->page_tables[i]) | 3;
    }
    return new;
}

int get_first_physical_page() {
    return bitmapSearch(physical_page_bitmap, 0, 0x100400, 0);
}

void* mark_physical_page_used(void * address) {
    bitmapSet(physical_page_bitmap, (int)address/0x1000);
}

void* mark_physical_page_free(void * address) {
    bitmapReset(physical_page_bitmap, (int)address/0x1000);
}

void map_page(PAGE_STRUCT* pages, uint32_t virtual, uint32_t physical) {
    mark_physical_page_used(physical);
    pages->page_tables[virtual/0x1000/1024][virtual/0x1000%1024] = physical / 0x1000 * 0x1000 | 3;
    bitmapSet(pages->bitmap,virtual/0x1000);
    bitmapSet(physical_page_bitmap,physical/0x1000);
}

void free_page(PAGE_STRUCT* pages, uint32_t virtual) {
    mark_physical_page_free(pages->page_tables[virtual/0x1000/1024][virtual/0x1000%1024]);
    pages->page_tables[virtual/0x1000/1024][virtual/0x1000%1024] = 0;
    bitmapReset(pages->bitmap,virtual/0x1000);
    bitmapReset(physical_page_bitmap,pages->page_tables[virtual/0x1000/1024][virtual/0x1000%1024]/0x1000);
}

void switch_cr3(void * new_cr3) {
    register uint32_t *page_directory_reference asm("eax");
    page_directory_reference = new_cr3;
    // load page directory
    __asm__("mov %eax, %cr3\n\t"
            "mov %cr0, %ebx\n\t"
            "or $0x80000000, %ebx\n\t"
            "mov %ebx, %cr0\n\t"
            "mov %cr3, %eax\n\t"
            "mov %eax, %cr3\n\t");

    (void)(page_directory_reference);
}