#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "libc/vstddef.h"

#define BACKSPACE 0x0E
#define ENTER 0x1C
#define SC_MAX 114

// Special Key Functions
#define SPECIAL_KEY_NUM 0x0F

#define LSHIFTF 0x00
#define LSHIFTP 0x2A
#define LSHIFTR 0xAA

#define RSHIFTF 0x01
#define RSHIFTP 0x36
#define RHISFTR 0xB6

#define LCTRL 0x1D

void init_keyboard(char * keybuffer, vf_ptr_s callback);
char * get_keybuffer();
vf_ptr_s get_callback();
uint8_t* get_keys_pressed();
char* read_line();

#endif