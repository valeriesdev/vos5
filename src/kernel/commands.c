/**
 * @defgroup   COMMANDS commands
 * @ingroup    KERNEL_FILES
 * @brief      This file provides functionality to register and recall commands by passing in a string.
 * 
 * @par
 * First, in the kernel initialization process, a command_block must be created. This serves as the head of a linked list containing all of the possible command pairs. Then, the head of that linked list, as well as the function pointer and the string that should call that function are passed into register_command in order to create the command.
 * @par
 * Then, just the linked list head and string can be passed into resolve_command  in order to retrieve the function pointer assocaited with the string.
 *
 * @author     Valerie Whitmire
 * @date       2023
 */
#include <stdint.h>
#include <stddef.h>
#include "kernel/commands.h"
#include "libc/string.h"
#include "libc/function.h"
#include "libc/mem.h"
#include "libc/vstddef.h"
#include "drivers/screen.h"
void NULLFUNC(char* args) { UNUSED(args); return; }

/**
 * @brief      Registers a new command into the linked list of command blocks.
 * @ingroup    COMMANDS
 * @param      command_resolver_head  The head of the command linked list.
 * @param[in]  new_function           A pointer to the function that should be associated with this command
 * @param      function_call_string   The string that, when resolved, will point to the new function
 */
void register_command(struct command_block *command_resolver_head, vf_ptr new_function, char* function_call_string) {
	struct command_block *current = command_resolver_head;
	while(current->next != NULL) {
		current = current->next;
	}

	struct command_block *command_block_new = malloc(sizeof(struct command_block)); // Does not need to be freed; should always stay in memory
	*command_block_new = (struct command_block) {.function = new_function, .call_string = function_call_string, .next = NULL};
	current->next = command_block_new;
}

/**
 * @brief      Resolves a string into a function pointer
 * @ingroup    COMMANDS
 * @param[in]  command_resolver_head  The head of the command linked list.
 * @param      function_call_string   The string which is being resolved.
 *
 * @return     a function pointer which takes in a char* and returns void.
 *             if string cannot be resolved, the NULLFUNC is returned.
 */
//void (*resolve_command(struct command_block command_resolver_head, char* function_call_string))(char*) {
vf_ptr_s resolve_command(struct command_block command_resolver_head, char* function_call_string) {
    struct command_block current = command_resolver_head;
	while(strcmp(current.call_string, function_call_string) != 0 && current.next != NULL) {
		current = *current.next;
	}

	if(strcmp(current.call_string, function_call_string) != 0) return NULLFUNC;
	return current.function;
}