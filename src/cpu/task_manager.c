#include "cpu/isr.h"
#include "cpu/paging.h"
#include "libc/mem.h"
#include <stddef.h>

typedef struct task {
	registers_t *regs; 
	paging_structure_t *assoc_paging_struc;
	struct task *next;
} TASK;

uint32_t num_tasks = 0;
uint32_t current_task = 0;
TASK* task_head = NULL;
uint8_t tasks_running = 0;
uint8_t tasking_enabled = 0;

void start_task(paging_structure_t * task_paging_struct, void (*run_from_address)(), uint8_t launch) ;
void next_task(registers_t *regs);
void switch_to_task(TASK task, registers_t *regs);
void switch_interrupt(registers_t *regs);
extern void irq_return(uint32_t a, uint32_t b, uint32_t c);

void start_task(paging_structure_t * task_paging_struct, void (*run_from_address)(), uint8_t launch) {
	tasking_enabled = 1;
	TASK* new_task;
	if(num_tasks > 0) {
		TASK* current = task_head;
		uint32_t i;
		for(i = 0; i < num_tasks-1; i++) 
			current = current->next;
		current->next = malloc(sizeof(TASK));
		num_tasks++;
		//current_task = num_tasks-1;
		new_task = current->next;
	} else {
		num_tasks++;
		task_head = malloc(sizeof(TASK));
		new_task = task_head;
	}



	if(task_paging_struct == NULL) { // setup paging
	
	} else { // use provided paging
		new_task->assoc_paging_struc = task_paging_struct;
		new_task->next = NULL;
	}

	new_task->regs = malloc(sizeof(registers_t));
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
	new_task->regs->eip = run_from_address;
	new_task->regs->cr3 = (uint32_t)(task_paging_struct->page_directory);
	new_task->regs->cs = 0x08;
	new_task->regs->eflags = 0x16;

	if(launch == 1) {
		run_from_address();
		return;
	} else {
		return;
	}
}

void next_task(registers_t *regs) {
	if(num_tasks == 0) return;
	else if(num_tasks == 1) {
		current_task = 0;
		switch_to_task(*task_head, regs);
		return;
	} else if(current_task == num_tasks-1) {
		current_task = 0;
		switch_to_task(*task_head, regs);
		return;
	}
	TASK* current = task_head;
	uint32_t i;
	for(i = 0; i <= current_task; i++) 
		current = current->next;
	current_task = i;
	switch_to_task(*current, regs);

    return;
}

// copys register values from task
void switch_to_task(TASK task, registers_t *regs) {	
	regs->ds = task.regs->ds;
	regs->cr3 = task.regs->cr3;
	regs->edi = task.regs->edi;
	regs->esi = task.regs->esi;
	regs->ebp = task.regs->ebp;
	regs->eax = task.regs->eax;
	regs->ebx = task.regs->ebx;
	regs->ecx = task.regs->ecx;
	regs->edx = task.regs->edx;
	regs->eip = task.regs->eip;
	regs->cs = task.regs->cs;
	//regs->eflags = task.regs->eflags;
	regs->esp = task.regs->esp;
	regs->ss = task.regs->ss;
    return;
}

// executes on every tick
void switch_interrupt(registers_t *regs) {
	if(tasks_running == 1) {
		TASK* current = task_head;
		uint32_t i;
		for(i = 0; i < current_task; i++) 
			current = current->next;
		*(current->regs) = *regs;
		next_task(regs);
	} else if(tasking_enabled == 1) {
		tasks_running = 1;
	}

    return;
}