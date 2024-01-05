#ifndef TASK_HANDLER_H
#define TASK_HANDLER_H

#include <stdint.h>

void start_process(void* load_from_address, void* entry_address, uint32_t length);
void reload_kernel();

#endif