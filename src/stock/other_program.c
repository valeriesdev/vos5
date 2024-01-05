#include "drivers/screen.h"
#include "cpu/task_handler.h"
#include <stdint.h>

#define header __attribute__((section(".other_header"))) 
#define entry __attribute__((section(".other_entry"))) 

struct fat_code {
	uint32_t magic[4];
	char name[32];
	uint32_t lba; // not needed
	uint32_t length; // not needed
} __attribute__((packed));

header struct fat_code other_file_info = {
	.magic = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF},
	.name = "other.vxv\0",
	.lba = 0,
	.length = 3
};

entry void other_func() {
	kprint("HELLO WORLD!!!");

	reload_kernel();
}