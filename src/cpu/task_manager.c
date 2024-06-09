#include "cpu/isr.h"
#include "cpu/paging.h"
#include "libc/mem.h"
#include "filesystem/filesystem.h"
#include "libc/vstddef.h"
#include "cpu/task_manager.h"
#include <stddef.h>
#include <stdatomic.h>

TASK tasks[256];
uint8_t num_tasks = 0;
uint8_t cur_task = 0;

// create a current task from the current page tables and cpu state
void insert_task(registers_t* regs) {
	tasks[num_tasks].regs = *regs;
	tasks[num_tasks].assoc_paging_struc = regs->ebx;
	cur_task = num_tasks;
	num_tasks++;
}

void switch_task(registers_t* regs) {
	if(num_tasks == 0) return;

	tasks[cur_task % num_tasks].regs = *regs;
	cur_task++;
	*regs = tasks[cur_task % num_tasks].regs;
	regs->cr3 = &(tasks[cur_task % num_tasks].assoc_paging_struc->page_directory);
}

void fork(registers_t *regs) {
	tasks[cur_task % num_tasks].regs = *regs;

	tasks[num_tasks].regs = *regs;
	tasks[num_tasks].assoc_paging_struc = regs->ebx;
	tasks[num_tasks].regs.eax = num_tasks;
    tasks[num_tasks].regs.eip = regs->ecx;
    
	regs->eax = 0;
	num_tasks++;
}

void setup_task_paging(registers_t *regs) {
	tasks[cur_task % num_tasks].assoc_paging_struc = copy_nonkernel_pages(&kernel_pages);
	uint8_t* stack_page = get_first_physical_page()*0x1000;
    map_page(tasks[cur_task % num_tasks].assoc_paging_struc, 0x5fff000, stack_page);
    tasks[cur_task % num_tasks].regs.esp = 0x5ffffff;
    tasks[cur_task % num_tasks].regs.ebp = 0x5ffffff;
}