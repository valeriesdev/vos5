#include "drivers/screen.h"
#include <stdint.h>

#include "kernel/kernel.h"

#define header __attribute__((section(".prime_header"))) 
#define entry __attribute__((section(".prime_entry"))) 

struct fat_code {
	uint32_t magic[4];
	char name[32];
	uint32_t lba; // not needed
	uint32_t length; // not needed
} __attribute__((packed));

header struct fat_code prime_file_info = {
	.magic = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF},
	.name = "prime.vxv\0",
	.lba = 0,
	.length = 1
};

entry void prime_entry() {
	kprintn("Finding all prime numbers up to 100000");
	uint32_t current_number = 2;
	for(; current_number < 10000; current_number++) {
		uint32_t current_divisor = 2;
		uint8_t is_prime = 1;
		for(; current_divisor < current_number/2+1; current_divisor++) {
			if(current_number % current_divisor == 0) {
				is_prime = 0;
				break;
			}
		}
		if(is_prime == 1) {
			kprint(int_to_ascii(current_number));
			kprintn(" is prime.");
		} else {

			//kprint(int_to_ascii(current_number));
			//kprintn(" is not prime.");
		}
		//save_task_state(0);
		//yield_switch();
	}

    //start_process(kernel_loop, 0, 0, 1);
	//reload_kernel();
}

