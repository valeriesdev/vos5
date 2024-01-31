#include "cpu/isr.h"
#include "cpu/paging.h"

void switch_interrupt(registers_t *regs);
void start_task(paging_structure_t * task_paging_struct, void (*run_from_address)(), uint8_t launch);