#include "cpu/isr.h"
#include "cpu/paging.h"
#include "libc/mem.h"
#include "filesystem/filesystem.h"
#include "libc/vstddef.h"
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
static void next_task();
static void switch_to_task(TASK task);
void switch_interrupt(registers_t *regs);
extern paging_structure_t kernel_paging_structure;

// kernel version
void start_task(paging_structure_t * task_paging_struct, void (*run_from_address)(), uint8_t launch) {
	tasking_enabled = 1;
	TASK* new_task;
	num_tasks++;
	new_task = &tasks[num_tasks-1];

	if(task_paging_struct == NULL) { // setup paging
		paging_structure_t * new_paging_struct = establish_paging_structure();
		new_task->assoc_paging_struc = new_paging_struct;

		/*if(((int)address & 0x00000FFF) != 0) {
			uint32_t free_page = find_first_frame();
			set_page_present(free_page*0x1000);
			memory_copy(address,(uint8_t*)(free_page*0x1000),length);
			address = free_page*0x1000;
		}*/

		uint32_t stack_page = find_first_frame();
		set_page_present(stack_page*0x1000);
		switch_paging_structure(new_paging_struct);
		set_page_value(0xF00000, (int)run_from_address | 0b011);
		set_page_value(0x8000000-0x1000, (int)(stack_page*0x1000) | 0b011);
		// run_from_address += offset;
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

	new_task->regs->ebp = (uint32_t) (ta_alloc(0x30000)+0x30000);
	new_task->regs->esp = new_task->regs->ebp-1;

	if(launch == 1) {
		current_task = num_tasks-1;
    	register_interrupt_handler(0x29, switch_interrupt);
    	run_from_address();
		return;
	} else {
		return;
	}
}

// user task
void start_user_task(uint16_t size, char *filename) {
	// load from disk
	// initialize page directory
	// mark code and data pages as okay
	// place task in tasking structures
	// return
	// 
	
	struct file_descriptor file = read_file(filename);
	void* run_from_address = file.address;


    paging_structure_t* new_pagetables = ta_alloc(sizeof(paging_structure_t));
    new_pagetables->page_directory     = ta_alloc_align(4096, 4096);
    new_pagetables->page_tables        = ta_alloc_align(1024*1024*4, 4096);
    new_pagetables->frame_bitmap       = ta_alloc(0x20000);
    new_pagetables->size               = 1024*1024;
    
    memory_set((uint8_t*)new_pagetables->frame_bitmap          , 0xFFFF, 0x20000);

	switch_paging_structure(new_pagetables);

	int i = 0, j = 0;
	for(i = 0; i < 1024; i++) {     // i corresponds to current page table
		for(j = 0; j < 1024; j++) { // j corresponds to current page
            new_pagetables->page_tables[1024*i+j] = kernel_paging_structure.page_tables[1024*i+j];
            if(test_frame((1024*i+j)*0x1000)) 
            	set_frame((1024*i+j)*0x1000);
            else
            	clear_frame((1024*i+j)*0x1000);
		}
		new_pagetables->page_directory[i] = ((unsigned int)&new_pagetables->page_tables[1024*i]) | 0b011; // supervisor rw present
	}

	if(((int)run_from_address & 0x00000FFF) != 0) {
		uint32_t free_page = find_first_frame();
		set_page_present(free_page*0x1000);
		memory_copy(run_from_address,(uint8_t*)(free_page*0x1000),1024);
		run_from_address = free_page*0x1000;
	}

	uint32_t stack_page = find_first_frame();
	set_page_present(stack_page*0x1000);
	switch_paging_structure(new_pagetables);
	set_page_value(0xF00000, (int)run_from_address | 0b011);
	set_page_value(0x8000000-0x1000, (int)(stack_page*0x1000) | 0b011);


	register uint32_t *page_directory_reference asm("eax");
	page_directory_reference = new_pagetables->page_directory;
	// load page directory
	__asm__("mov %eax, %cr3\n\t"
            "mov %cr0, %ebx\n\t"
		    "or $0x80000000, %ebx\n\t"
		    "mov %ebx, %cr0\n\t"
            "mov %cr3, %eax\n\t"
		    "mov %eax, %cr3\n\t");

    (void)(page_directory_reference);
    ((vf_ptr)(0xF00000+56))();

}

static void next_task() {
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
static void switch_to_task(TASK task) {   
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
    	"add $32, %%esp\n\t"
	 	"jmp -20(%%esp)" : : "m"((task.regs->esp)), "m"(task.regs->ebp));
}

// executes on every tick
void switch_interrupt(registers_t *regs) {
	if(num_tasks == 1) return;
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
	while(atomic_flag_test_and_set(mutex))
	{
		__builtin_ia32_pause();
	}
}
 
void release_mutex(atomic_flag* mutex)
{
	atomic_flag_clear(mutex);
}