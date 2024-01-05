/**
 * @defgroup   KERNEL_FILES kernel
 **/
/**
 * @defgroup   BASIC_COMMANDS basic commands
 * @ingroup    KERNEL_FILES
 * @brief      This file holds the basic kernel commands.
 * 
 * @note       Commands should pass in a char *args, regardless of whether or not they use it. If not, use UNUSED(args) in the function.
 *
 * @author     A
 * @date       2023
 */
#include <stdint.h>
#include <stddef.h>
#include "kernel/commands.h"
#include "libc/string.h"
#include "libc/mem.h"
#include "drivers/screen.h"
#include "libc/function.h"
#include "filesystem/filesystem.h"
#include "cpu/task_handler.h"

extern struct command_block *command_resolver_head;

void ECHO(char *args) {
	kprint(args);
	kprint("\n");

	UNUSED(args);
}

void END(char *args) {
	kprint("Stopping the CPU. Bye!\n");
    asm volatile("hlt");

    UNUSED(args);
}

void PAGE(char *args) {
    void* page = malloc(1000);
    kprint("Page: ");
    kprintn(hex_to_ascii((int)page));

    UNUSED(args);
}

void LS(char *args) {
	//kprint("\n");
	struct file* files = get_files()+1;
	while(files->magic == 0xFFFFFFFF) {
		kprintn(files->name);
		files++;
	}

	UNUSED(args);
}

void HELP(char *args) {
	struct command_block *t = (command_resolver_head->next);
	while(t != NULL) {
		kprintn(t->call_string);
		t = t->next;
	}

	UNUSED(args);
}

void DEBUG_PAUSE(char *args) {
	/*int i = 0;
	for(i = 0; i < 512*10; i += 1) {
		void* z = malloc(i);
		char* x = int_to_ascii(i);
		kprintn(x);
		free(z);
		free(x);
	}*/

	//debug_traverse();

	UNUSED(args);
}

void RUN(char *args) {
	struct file_descriptor file = read_file(args);
	void* program = file.address;
	if(program != 0) {
		start_process(program, program+0x38, file.size_bytes);
	} else {
		kprint("Program not found.\n");
	}
}

void LESS(char *args) {

	UNUSED(args);
}