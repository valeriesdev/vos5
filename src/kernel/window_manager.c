#include "drivers/screen.h"

void setup_windows() {
	//clear_screen();
	int i = 0;
	for(i = 0; i < 79; i++) {
		kprint_at_preserve("-", i, 24);
	}
}

void wprint(char *message, uint8_t window) {
	if(window == 0) {
		kprint(message);
	} else {
		kprint(message);
	}
}