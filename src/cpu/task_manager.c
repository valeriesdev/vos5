#include <stdint.h>
#include "cpu/paging.h"
#include "libc/mem.h"

typedef struct process_information {
	uint32_t pid;                    // Set on init
	paging_structure_t *page_structure; // ^
	uint32_t ebp;                    // ^
	uint32_t esp;                    // Set on swap away from
	uint32_t eip;                    // ^
	uint32_t cr3;                    // ^ + probably unneeded
} PROCESS_T;

uint32_t current_process_pid = -1;
PROCESS_T *live_processes[2];

extern uint8_t is_alternate_process_running;
extern paging_structure_t kernel_paging_structure;

// This will initialize a process, assign it a value in live_processes, set up it's paging, and jump to  it
void start_process(void *address, void *entry_offset, uint32_t length, uint8_t kernel_process);

// This will free all of the process's assocaited values and delete its entry in the live_processes
void kill_process(uint32_t pid);

// This will swap a process
void switch_process(uint32_t pid);

void start_process(void *address, void *entry_offset, uint32_t length, uint8_t kernel_process) {
	if(kernel_process == 1) start_process_kernel(address);
	else start_process_user(address, entry_offset, length);
}

void start_process_kernel(void *address) {
	if(current_process_pid == -1) current_process_pid = 0;
	else if(current_process_pid == 0) current_process_pid = 1;
	else current_process_pid = 0;
	uint32_t this_pid = current_process_pid;
	live_processes[this_pid] = malloc(sizeof(PROCESS_T));

	live_processes[this_pid]->pid = this_pid;
	live_processes[this_pid]->page_structure = &kernel_paging_structure;

	// Load our new directory
	is_alternate_process_running = 0;
	
	switch_paging_structure(&kernel_paging_structure);
	__asm__("cli\n\t"
		    "mov %0, %%ecx\n\t"
		    "mov %1, %%cr3\n\t"
		    "mov %%cr3, %%edx\n\t"
		    "mov %%edx, %%cr3\n\t"
		    "sti\n\t"
		    "jmp %%ecx\n\t"
		    	: : "r"(address), "r"(kernel_paging_structure.page_directory));
}

void start_process_user(void *address, void *entry_offset, uint32_t length) {
	if(current_process_pid == -1) current_process_pid = 0;
	else if(current_process_pid == 0) current_process_pid = 1;
	else current_process_pid = 0;
	uint32_t this_pid = current_process_pid;
	live_processes[this_pid] = malloc(sizeof(PROCESS_T));

	live_processes[this_pid]->pid = this_pid;
	live_processes[this_pid]->page_structure = malloc(sizeof(paging_structure_t));
	live_processes[this_pid]->page_structure->page_directory = malloc_align(4096, 4096);
	live_processes[this_pid]->page_structure->page_tables = malloc_align(1024*1024*4, 4096);
	live_processes[this_pid]->page_structure->frame_bitmap = malloc(0x20000);
	live_processes[this_pid]->page_structure->size = 1024*1024;

	// If the address is not page aligned, we will get a free page and move it there
	if(((int)address & 0x00000FFF) != 0) {
		uint32_t free_page = find_first_frame();
		set_page_present(free_page*0x1000);
		memory_copy(address,(uint8_t*)(free_page*0x1000),length);
		address = free_page*0x1000;
	}

	uint32_t stack_page = find_first_frame();
	set_page_present(stack_page*0x1000);

	uint32_t *tables = live_processes[this_pid]->page_structure->page_tables;
	uint32_t *page_dir = live_processes[this_pid]->page_structure->page_directory;
	uint32_t *kernel_tables = kernel_paging_structure.page_tables;
	switch_paging_structure((live_processes[this_pid]->page_structure));
	int i, j;
	for(i = 0; i < 1024; i++) {     // i corresponds to current page table
		for(j = 0; j < 1024; j++) { // j corresponds to current page
			tables[1024*i+j] = kernel_tables[1024*i+j];  // copy the kernel page table, prob repalce w/ memcpy
			if(tables[1024*i+j] == ((1024*i+j)*0x1000) | 0b010) clear_frame((1024*i+j)*0x1000);
			else set_frame((1024*i+j)*0x1000);
		}
		page_dir[i] = (int)((unsigned int)&tables[1024*i]) | 0b011; // supervisor rw present
	}
	set_page_value(0xF00000, (int)address | 0b011);
	set_page_value(0x8000000-0x1000, (int)(stack_page*0x1000) | 0b011);
	// Load our new directory
	uint32_t eip = 0xF00000 + (uint32_t)entry_offset;
	is_alternate_process_running = 1;
	kprint_at("h", 0, 26);
	kprint_backspace();
	__asm__("cli\n\t"
		    "mov %0, %%ecx\n\t"
		    "mov %1, %%cr3\n\t"
		    "mov %%cr3, %%edx\n\t"
		    "mov %%edx, %%cr3\n\t"
		    "sti\n\t"
		    "jmp %%ecx\n\t"
		    	: : "r"(eip), "r"(page_dir));
}

void kill_process(uint32_t pid) {
}

void switch_process(uint32_t pid) {
	
}