#ifndef KERNEL_H
#define KERNEL_H

extern struct command_block *command_resolver_head;
void user_input(char *input); // remove from header?
void kernel_init_keyboard();
void kernel_loop();

#endif
