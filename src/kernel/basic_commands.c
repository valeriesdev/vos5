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
#include "cpu/task_manager.h"
extern struct command_block *command_resolver_head;
extern void* kernel_paging_structure;

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
	UNUSED(args);
}

void RUN(char *args) {
	struct file_descriptor file = read_file(args);
	void* program = file.address;
	if(program != 0) {
		//start_process(program, 0x38, file.size_bytes, 0);
		//start_program(program, 1, 0x38, file.size_bytes);
		start_task((paging_structure_t*)&kernel_paging_structure, program, 0);
	} else {
		kprint("Program not found.\n");
	}

	return;
}

void LESS(char *args) {

	UNUSED(args);
}