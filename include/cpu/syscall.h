#include "cpu/paging.h"

int enable_syscalls();
int sys_fork(PAGE_STRUCT *paging_struct);
int sys_insert_task(PAGE_STRUCT *paging_struct);
int sys_stp();