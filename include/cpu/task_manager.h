// This will initialize a process, assign it a value in live_processes, set up it's paging, and jump to  it
void start_process(void *address, void *entry_offset, uint32_t length, uint8_t kernel_process);

// This will free all of the process's assocaited values and delete its entry in the live_processes
void kill_process(uint32_t pid);

// This will swap a process
void switch_process(uint32_t pid);

extern void yield();