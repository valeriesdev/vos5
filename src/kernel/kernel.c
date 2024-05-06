/**
 * @defgroup   KERNEL kernel
 * @ingroup    KERNEL_FILES
 * @brief      The operating system kernel entry point
 * @todo       Replace get_keybuffer() calls with reference to local keybuffer
 * 
 * @author     Valerie Whitmire
 * @date       2023
 */
#include <stdint.h>
#include "cpu/isr.h"
#include "drivers/screen.h"
#include "drivers/ata.h"
#include "libc/function.h"
#include "libc/string.h"
#include "libc/mem.h"
#include "libc/vstddef.h"
#include "kernel/kernel.h"
#include "kernel/commands.h"
#include "filesystem/filesystem.h"
#include "drivers/keyboard.h"
#include "cpu/timer.h"
#include "cpu/paging.h"
#include "cpu/ports.h"
#include "kernel/windows.h"
#include "cpu/task_manager.h"

struct command_block *command_resolver_head;
char **lkeybuffer = NULL;
vf_ptr_s next_function = NULL;

/**
 * @brief      The kernel entry point.
 * @ingroup    KERNEL
 */
extern uint32_t address_linker;
extern uint32_t length_linker;

extern void* kernel_paging_structure;
atomic_flag i_mutex = ATOMIC_FLAG_INIT;

int i = 0;

void printb() {
    acquire_mutex(&i_mutex);
    while(1) {
        i = 0;
        while(i < 1000000000) {
            if(i%100000 == 0) {
                release_mutex(&i_mutex);
                char *z = int_to_ascii(i);
                kprint_at_preserve(z, 60, 10);
                ta_free(z);
                int j;
                for(j = 0; j < 1000000; j++);
                acquire_mutex(&i_mutex);
            }
            i++;
        }
    }
}

void printa() {
    acquire_mutex(&i_mutex);
    while(1) {
        i = 0;
        while(i < 1000000000) {
            if(i%100000 == 0) {
                release_mutex(&i_mutex);char *z = int_to_ascii(i);
                kprint_at_preserve(z, 60, 9);
                ta_free(z);
                int j;
                for(j = 0; j < 1000000; j++);
                acquire_mutex(&i_mutex);
            }
            i++;
        }
    }
}

__attribute__((section(".kernel_entry")))  void kernel_main() {
    ta_init(0x100000, 0xeff000, 256, 16, 8);
    isr_install();
    irq_install();

    lkeybuffer = ta_alloc(256);
    init_keyboard(lkeybuffer, NULL);

    enable_paging();
    init_fat_info();
    load_fat_from_disk();

    command_resolver_head = ta_alloc(sizeof(struct command_block)); // Does not need to be ta_freed; should always stay in memory
    command_resolver_head->function = NULLFUNC;
    command_resolver_head->call_string = "";
    command_resolver_head->next = NULL;

    register_command(command_resolver_head, END, "end");
    register_command(command_resolver_head, PAGE, "page");
    register_command(command_resolver_head, ECHO, "echo");
    register_command(command_resolver_head, LS, "ls");
    register_command(command_resolver_head, HELP, "help");
    register_command(command_resolver_head, DEBUG_PAUSE, "debug_command");
    register_command(command_resolver_head, RUN, "run");

    //setup_windows();
    start_task((paging_structure_t*)&kernel_paging_structure, printb, 0);
    start_task((paging_structure_t*)&kernel_paging_structure, printa, 0);
    start_task((paging_structure_t*)&kernel_paging_structure, kernel_loop, 1);
    while(1);
}


void kernel_loop() {
    next_function = NULL;
    kprint("> ");
    get_keybuffer()[0] = '\0';
    while(1) {
        char* input = read_line();
        next_function = resolve_command(*command_resolver_head, str_split(input, ' ')[0]);
        if(next_function != NULL) {
            kprint("\n");
            char** args = str_split(input, ' ');
            char* args_processed = ta_alloc(sizeof(char)*30);
            int current_arg = 1;
            while(args[current_arg] != 0x0 && args[current_arg] != '\0') {
                int i = 0;
                for(i = 0; i < strlen(args[current_arg]); i++) 
                    append(args_processed, args[current_arg][i]);
                current_arg++;
            }

            next_function(args_processed);
            next_function = NULL;            
            kprint("> ");
            get_keybuffer()[0] = '\0';
            ta_free(args_processed);
        }
    }
}