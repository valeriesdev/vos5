#include "drivers/screen.h"

static uint32_t left_cursor_col = 0;
static uint32_t left_cursor_row = 0;
static uint32_t right_cursor_col = 0;
static uint32_t right_cursor_row = 0;

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