#include "cpu/isr.h"
#include "cpu/paging.h"
#include "libc/mem.h"
#include <stddef.h>
#include <stdatomic.h>

typedef struct task {
	registers_t *regs; 
	paging_structure_t *assoc_paging_struc;
} TASK;

/**
 * Global Variables
 */
TASK tasks[256];
uint32_t num_tasks = 0;
uint32_t current_task = 0;
uint8_t tasks_running = 0;
uint8_t tasking_enabled = 0;
atomic_flag kernel_mutex = ATOMIC_FLAG_INIT;

/**
 * Function definitions
 */
void start_task(paging_structure_t * task_paging_struct, void (*run_from_address)(), uint8_t launch) ;
void next_task();
void switch_to_task(TASK task);
void switch_interrupt(registers_t *regs);

void start_task(paging_structure_t * task_paging_struct, void (*run_from_address)(), uint8_t launch) {
	tasking_enabled = 1;
	TASK* new_task;
	num_tasks++;
	new_task = &tasks[num_tasks-1];

	if(task_paging_struct == NULL) { // setup paging
	
	} else { // use provided paging
		new_task->assoc_paging_struc = task_paging_struct;
	}

	new_task->regs = ta_alloc(sizeof(registers_t));
	asm("mov %%ds, %0\n\t"
   	   //"mov %%cr3, %1\n\t"
   	   "mov %%edi, %1\n\t"
   	   "mov %%esi, %2\n\t"
   	   "mov %%ebp, %3\n\t"
   	   "mov %%cs, %4\n\t"
   	   "mov %%esp, %5\n\t"
   	   "mov %%ss, %6\n\t"
   	   /*"jmp *%9"*/      :"=r"(new_task->regs->ds),  "=m"(new_task->regs->edi),
   	   					   "=m"(new_task->regs->esi), "=m"(new_task->regs->ebp),
   	   					   "=m"(new_task->regs->cs),  "=m"(new_task->regs->esp), 
   	   					   "=m"(new_task->regs->ss) :);
	new_task->regs->eip = (uint32_t)run_from_address;
	new_task->regs->cr3 = (uint32_t)(task_paging_struct->page_directory);
	new_task->regs->cs = 0x08;
	asm("pushf\n\t pop %0" : "=m"(new_task->regs->eflags));

	new_task->regs->ebp = (uint32_t) (ta_alloc(0x10000)+0x10000);
	new_task->regs->esp = new_task->regs->ebp-1;

	if(launch == 1) {
		//current_task = num_tasks-1;
    	//register_interrupt_handler(0x29, switch_interrupt);
    	//run_from_address();
		return;
	} else {
		return;
	}
}

void next_task() {
	if(num_tasks == 0) return;
	else if(num_tasks == 1) {
		current_task = 0;
		switch_to_task(tasks[0]);
		return;
	} else if(current_task == num_tasks-1) {
		current_task = 0;
		switch_to_task(tasks[0]);
		return;
	}
	current_task++;
	switch_to_task(tasks[current_task]);
    return;
}

// copys register values from task
void switch_to_task(TASK task) {

	//task.regs->eflags = regs->eflags;
	task.regs->esp -= sizeof(registers_t);
	memory_copy((uint8_t*)task.regs, (uint8_t*)task.regs->esp,sizeof(registers_t));
 
 
	asm("mov %0, %%esp\n\t"
	 	"mov %1, %%ebp\n\t"
	 	"pop %%ebx\n\t"
	    "mov %%bx, %%ds\n\t"
	    "mov %%bx, %%es\n\t"
	    "mov %%bx, %%fs\n\t"
	    "mov %%bx, %%gs\n\t"
    	"pop %%eax\n\t" 
    	"mov %%eax, %%cr3\n\t"
    	"popa\n\t"
	 	//"pop %%ebx\n\t" // clobber ebx
    	"add $32, %%esp\n\t"
	 	"jmp -20(%%esp)" : : "m"((task.regs->esp)), "m"(task.regs->ebp));
}

// executes on every tick
void switch_interrupt(registers_t *regs) {
	if(tasks_running == 1) {
		*(tasks[current_task].regs) = *regs;
		tasks[current_task].regs->esp = regs->nesp;
		regs->eip = (uint32_t) next_task;
		return;
	} else if(tasking_enabled == 1) {
		tasks_running = 1;
		regs->eip = (uint32_t) next_task;
	}

    return;
}

 
void acquire_mutex(atomic_flag* mutex)
{
//	while(atomic_flag_test_and_set(mutex))
//	{
//		__builtin_ia32_pause();
//	}
}
 
void release_mutex(atomic_flag* mutex)
{
//	atomic_flag_clear(mutex);
}