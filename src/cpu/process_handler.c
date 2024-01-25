#include <stdint.h>
#include "cpu/paging.h"
#include "libc/mem.h"

typedef struct registers_state {
	uint32_t ebp;
	uint32_t esp;
	uint32_t cr3;
} REGISTERS_STATE_T;

typedef struct process_info {
	uint32_t pid;
	void* entry_point;
	paging_structure_t* paging_structure;
} PROCESS_T;

REGISTERS_STATE_T *inactive_program_states = NULL;
PROCESS_T* processes = NULL;
extern paging_structure_t **all_paging_structures;
extern paging_structure_t kernel_paging_structure;
uint8_t is_alternate_process_running;


/**
 * @brief      Simply re-enters the kernel. This only works for singletasking mode
 */
void reload_kernel() {
	is_alternate_process_running = 0;
	switch_paging_structure(&kernel_paging_structure);
	__asm__("cli\n\t"
		    //"mov %0, %%esp\n\t"
		    //"mov %1, %%ebp\n\t"
		    "mov %0, %%cr3\n\t"
		    "mov %%cr3, %%edx\n\t"
		    "mov %%edx, %%cr3\n\t"
		    "sti\n\t"
		    	: : "r"(kernel_paging_structure.page_directory));

	
	clear_screen();
	kprint("\n> ");
	
	kernel_init_keyboard();
	kernel_loop();
}