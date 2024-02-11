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

void printb() {
    while(1) {
        int i = 0;
        while(i < 100000000) {
            void * z = ta_alloc(700);
                kprint_at_preserve("process b running ", 60, 9);
                kprint_at_preserve(int_to_ascii(i), 60, 10);
                //yield();
            i++;
            ta_free(z);
        }
    }
}

void printa() {
    while(1) {
        int i = 0;
        while(i < 100000000) {
            if(i%1000000 == 0) {
                kprint_at_preserve("process a running ", 60, 5);
                kprint_at_preserve(int_to_ascii(i), 60, 6);
                //yield();
            }
            i++;
        }
    }
}

__attribute__((section(".kernel_entry")))  void kernel_main() {
    ta_init(0x100000, 0xf00000, 256, 16, 8);
    isr_install();
    irq_install();
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

    setup_windows();

    //printb();
    start_task((paging_structure_t*)&kernel_paging_structure, printa, 0);
    start_task((paging_structure_t*)&kernel_paging_structure, printb, 0);
    start_task((paging_structure_t*)&kernel_paging_structure, kernel_loop, 1);
    while(1);
}

void kernel_loop() {
    next_function = NULL;
    kprint("> ");
    get_keybuffer()[0] = '\0';
    clear_screen();
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
                if(args[current_arg] != '\0')
                append(args_processed, ' ');
            }

            next_function(args_processed);
            next_function = NULL;            
            kprint("> ");
            get_keybuffer()[0] = '\0';
            ta_free(args_processed);
        }
    }
}

/**
 * @brief      Initializes the keyboard to the state it should be in for CLI use
 * @ingroup    KERNEL
 */
void kernel_init_keyboard() {
    if(lkeybuffer != NULL) {
        ta_free(lkeybuffer);
        lkeybuffer = NULL;
    }
    lkeybuffer = ta_alloc(sizeof(char)*256);
    uint8_t keycodes[] = {0x0, 0x0, 0x0};
    void (*gcallback_functions[])() = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    struct keyboard_initializer* keyboardi = create_initializer(1,
                                                                keycodes,
                                                                gcallback_functions,
                                                                0x0,
                                                                (char*) lkeybuffer); // ??? Why does casting from a char** to a char* just work???);
    init_keyboard(keyboardi);
}