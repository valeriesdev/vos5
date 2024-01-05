#include "cpu/paging.h"
#include "filesystem/filesystem.h"
#include "libc/mem.h"
#include "kernel/kernel.h"
#include <stdint.h>

static void initialize_tables (uint32_t *page_dir, uint32_t *tables);

static uint32_t* new_page_directory = NULL;
static uint32_t* new_page_tables = NULL;
static void* last_file = NULL;
static uint32_t bitmap = NULL;

void start_process(void* load_from_address, void* entry_address, uint32_t length) {
	// Duplicate the page directory
	// Grab an empty page (old page directory)
	// Identity map the page (old page directory)
	// Load program into that page (old page directory)
	// Map your page(s) to 0xf00000 (new page directory page directory)
	// Load our new directory
	// 
	// Duplicate the page directory
	if(new_page_directory == NULL) new_page_directory = malloc_align(4096, 4096);
	if(new_page_tables == NULL) new_page_tables = malloc_align(1024*1024*4, 4096);
	if(last_file != NULL) free(last_file);
	last_file = load_from_address;

	initialize_tables(new_page_directory, new_page_tables);

	paging_structure_t new_page_structure = {
		.page_directory = new_page_directory,
		.page_tables = new_page_tables,
		.frame_bitmap = NULL, // we don't have a frame bitmap yet, set this up
		.size = 1024*1024
	};

	// Grab an empty page and set it present
	uint32_t free_page = find_first_frame();
	set_page_present(free_page*0x1000);
	// Load program into that page
	memory_copy(load_from_address,(uint8_t*)(free_page*0x1000),length);
	// Remap our page(s) to 0xf00000
	switch_paging_structure(&new_page_structure);      // From here on, we are using our new pages
	set_page_value(0xF00000, free_page*0x1000|0b011);
	// Load our new directory
	uint32_t eip = 0xF00000 + entry_address - load_from_address;

	__asm__("cli\n\t"
		    //"mov %0, %%esp\n\t"
		    //"mov %1, %%ebp\n\t"
		    "mov %0, %%ecx\n\t"
		    "mov %1, %%cr3\n\t"
		    "mov %%cr3, %%edx\n\t"
		    "mov %%edx, %%cr3\n\t"
		    "sti\n\t"
		    "jmp %%ecx\n\t"
		    	: : /*"r"(esp),*/ "r"(eip), /*"r"(ebp),*/ "r"(new_page_directory));
}

static void initialize_tables (uint32_t *page_dir, uint32_t *tables) {
	int i = 0, j = 0;
	for(i = 0; i < 1024; i++) {     // i corresponds to current page table
		for(j = 0; j < 1024; j++) { // j corresponds to current page
            tables[1024*i+j] = ((1024*i+j)*0x1000) | 0b011;  // supervisor rw not present
		}
		page_dir[i] = ((unsigned int)&tables[1024*i]) | 0b011; // supervisor rw present
	}
}

void reload_kernel() {
	//update_last_paging_structure();
	switch_paging_structure(&kernel_paging_structure);
	__asm__("cli\n\t"
		    //"mov %0, %%esp\n\t"
		    //"mov %1, %%ebp\n\t"
		    "mov %0, %%cr3\n\t"
		    "mov %%cr3, %%edx\n\t"
		    "mov %%edx, %%cr3\n\t"
		    "sti\n\t"
		    	: : "r"(kernel_paging_structure.page_directory));
	//free_paging_structure();

	kernel_init_keyboard();
	kernel_loop();
}