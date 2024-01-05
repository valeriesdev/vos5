#include <stdint.h>
#include "drivers/keyboard.h"
#include "drivers/screen.h"
#include "kernel/kernel.h"
#include "libc/mem.h"
#include "libc/string.h"
#include "filesystem/filesystem.h"
#include "stock/tedit.h"
#include "stock/program_interface/popup.h"

#include "cpu/task_handler.h"

#define TRUE 1
#define FALSE 0

#define tedit_header __attribute__((section(".tedit_header"))) 
#define tedit_code __attribute__((section(".tedit_code"))) 

// Functions
static void initialize_keyboard();
static void exit_program();
static void initialize();
static void save_program();

struct fat_code {
	uint32_t magic[4];
	char name[32];
	uint32_t lba; // not needed
	uint32_t length; // not needed
} __attribute__((packed));

tedit_header struct fat_code file_info = {
	.magic = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF},
	.name = "tedit.vxv\0",
	.lba = 0,
	.length = 3
};

static const char a[] = "woah... this is a new program.\n I think im loaded at 0xF000000...\n but i'm actually loaded at 0x3003000\0";

tedit_code void func()  {
	initialize();
}
// Variables that will need to be malloc'd and free'd
char* keybuffer = NULL;

// Variables
uint8_t new_file = NULL;
char *file_name = NULL;
uint8_t exit = NULL;

static void initialize_keyboard() {
	keybuffer = malloc(sizeof(char)*256);
	void (*gcallback_functions[])() = {exit_program, save_program, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	uint8_t keycodes[] = {0x1D, 0x10, 0x0, 0x1D, 0x1F, 0x0};
    
    struct keyboard_initializer* keyboardi = create_initializer(2, keycodes, gcallback_functions, 0x0, keybuffer);
    init_keyboard(keyboardi);
}

static void exit_program() {
	// Free all memory related to program
	keybuffer = free(keybuffer);

	// Prepare return to CLI
	clear_screen();
	kprint("\n> ");

	// Init CLI Keyboard
	//kernel_init_keyboard();
	exit = 0;
	reload_kernel();
}

static void save_program() {
	keybuffer[255] = '\0';
	if(new_file) {
		write_file(file_name, keybuffer, strlen(keybuffer));
	} else {
		overwrite_file(file_name, keybuffer, strlen(keybuffer));
	}
	
	backspace(keybuffer);
}

static void initialize() {
	clear_screen();
	initialize_keyboard();

	struct popup_str_struct* z = malloc(sizeof(struct popup_str_struct));
	*z = (struct popup_str_struct) {5,20,5,15,6,"Enter file name: ", 12, NULL};
	file_name = create_popup(1, z);
	z = free(z);

	// Static screen text
	char* header_00 = "0x00000 | VOS TEdit 0.0 | File: ";
	char* header_01 = " | ? ";
	char* footer_00 = "Press ctrl+q to exit, ctrl+s to save.";

	kprint_at(header_00,0,0);
	kprint(file_name);
	kprint(header_01);
	kprint_at_preserve(footer_00,0,24);
	kprint("\n");

	struct file* files = get_files()+1;
	while(files->magic == 0xFFFFFFFF) {
		if(strcmp(files->name,file_name) == 0) break;
		files++;
	}
	if(files->magic != 0xFFFFFFFF) {
		// procedure for new file
		new_file = TRUE;
	} else {
		// procedure for editing file
		new_file = FALSE;
		void* old_file = read_file(file_name).address;
		memory_copy((uint8_t*)old_file,(uint8_t*)keybuffer,strlen((char*)old_file));
		kprint_at(keybuffer,0,1);
		free(old_file);
	}

	exit = 1;
	while(exit);
}