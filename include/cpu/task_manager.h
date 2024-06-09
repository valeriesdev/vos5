#include "cpu/isr.h"
#include "cpu/paging.h"
#include "cpu/isr.h"
#include <stdatomic.h>

typedef struct task {
	registers_t regs; 
	PAGE_STRUCT *assoc_paging_struc;
} TASK;

TASK tasks[256];

void switch_task(registers_t* regs);
void insert_task(registers_t* regs);
void fork(registers_t* regs);
void setup_task_paging(registers_t *regs);