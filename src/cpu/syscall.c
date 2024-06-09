#include "cpu/syscall.h"
#include "cpu/isr.h"
#include "cpu/task_manager.h"

void syscall(registers_t* regs);

int enable_syscalls() {
	register_interrupt_handler(33, syscall);
}

void syscall(registers_t* regs) {
	switch_cr3(&kernel_pages.page_directory);
	switch (regs->eax) {
		case 0:
			insert_task(regs);
			break;
		case 1:
			fork(regs);
			break;
		case 2:
			setup_task_paging(regs);
			break;
	}
}

// eax = 0
// ebx = address of paging structure
int sys_insert_task(PAGE_STRUCT *paging_struct) {
    register uint32_t *eax asm("eax");
    register uint32_t *ebx asm("ebx");
    eax = 0;
    ebx = paging_struct;
    __asm__("int $33");
}

// eax = 1
int sys_fork(PAGE_STRUCT *paging_struct) {
	register uint32_t *eax asm("eax");
    register uint32_t *ebx asm("ebx");
    register uint32_t *ecx asm("ecx");
    ecx = __builtin_return_address(0);
    ebx = paging_struct;
    eax = 1;
    __asm__("int $33");
}

int sys_stp() {
    __asm__("int $33");
}