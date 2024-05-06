#include <stdint.h>
#include <stddef.h>
#include "drivers/keyboard.h"
#include "drivers/screen.h"
#include "cpu/ports.h"
#include "cpu/isr.h"
#include "libc/string.h"
#include "libc/function.h"
#include "kernel/kernel.h"
#include "libc/mem.h"
#include "cpu/timer.h"
#include "cpu/task_manager.h"

char* key_buffer = NULL;
vf_ptr_s key_callback = NULL;

const char ascii[] =       {'?','?','1','2','3','4','5','6','7','8','9',
                            '0','-','=','?','?','q','w','e','r','t',
                            'y','u','i','o','p','[',']','?','?','a',
                            's','d','f','g','h','j','k','l',';','\'',
                            '`','?','\\','z','x','c','v','b','n','m',
                            ',','.','/','?','?','?',' ','?','?','?'};
const char ascii_shift[] = {'?','?','!','@','#','$','%','^','&','*','(',
                            ')','_','+','?','?','Q','W','E','R','T',
                            'Y','U','I','O','P','{','}','?','?','A',
                            'S','D','F','G','H','J','K','L',':','\"',
                            '~','?','\\','Z','X','C','V','B','N','M',
                            '<','>','?','?','?','?',' ','?','?','?'};
uint8_t keys_pressed[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0};



static void default_keyboard_callback(registers_t *regs) { 
    uint8_t scancode = port_byte_in(0x60);

    if(scancode < 0x81) keys_pressed[scancode] = 1;
    else                keys_pressed[scancode-0x80] = 0;

    if(scancode < 0x81) {
        if (scancode == BACKSPACE) {
            backspace(key_buffer);
            kprint_backspace();
        } else if (scancode == LSHIFTP) {
        } else if (scancode == RSHIFTP) {
        } else if (scancode == LCTRL) {
        }  else if (scancode == 0x1C) {
            append(key_buffer, 0x1C); 
        } else {                                                    
            char letter = (keys_pressed[0x2A] || keys_pressed[0x36]) ? ascii_shift[(int) scancode] : ascii[(int) scancode];
            char str[2] = {letter, '\0'}; 
            append(key_buffer, letter); 
            kprint(str);
        }

        // char letter = (keys_pressed[0x2A] || keys_pressed[0x36]) ? ascii_shift[(int) scancode] : ascii[(int) scancode];
        // char str[2] = {letter, '\0'}; 
        // kprint(str);
    }

    if(key_callback != NULL) key_callback(key_buffer);

    UNUSED(regs);
}


void init_keyboard(char * keybuffer, vf_ptr_s callback) {
	key_buffer = keybuffer;
	key_callback = callback; 
	register_interrupt_handler(IRQ1, default_keyboard_callback);
}

char* get_keybuffer() {
	return key_buffer;
}

vf_ptr_s get_callback() {
	return key_callback;
}

uint8_t* get_keys_pressed() {
	return keys_pressed;
}

char* read_line() {
	char * old_keybuffer = get_keybuffer();
	vf_ptr_s old_callback = get_callback();
	char* line_keybuffer = ta_alloc(256);
	init_keyboard(line_keybuffer, NULL);

    while(1) {
        //kprint_at_preserve(line_keybuffer,1, 1);
        if(character_exists(0x1C, line_keybuffer) > -1) break;
    }
    append(line_keybuffer, '\0');
    line_keybuffer[strlen(line_keybuffer)-1] = '\0';

    init_keyboard(old_keybuffer, old_callback);

    return line_keybuffer;
}

void await_keypress() {
    char * old_keybuffer = get_keybuffer();
	vf_ptr_s old_callback = get_callback();
	char* line_keybuffer = ta_alloc(256);

	init_keyboard(line_keybuffer, NULL);

    while(1) if(strlen(line_keybuffer) > 0) break;

    ta_free(line_keybuffer);
    init_keyboard(old_keybuffer, old_callback);
}