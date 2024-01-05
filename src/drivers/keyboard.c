/**
 * @defgroup   KEYBOARD keyboard
 * @ingroup    DRIVERS
 * @brief      This file implements a PS/2 keyboard driver.
 * 
 * @par
 * To utilize the keyboard, a program must first establish and install a keyboard initializer. To create the initializer, the create_initializer function can be used. This initializer should then be passed to init_keboard.
 * The keyboard initialization process is too convoluted and needs to be overhauled, and has several memory management issues.
 * 
 * To establish and install the keyboard initializer, follow the following example:
 * @code
 *  keybuffer = malloc(sizeof(char)*256);                      // Keystrokes will be stored here
    uint8_t *keycodes = malloc(sizeof(uint8_t)*3);             // Memory management issue
    keycodes[0] = 0x1C; keycodes[1] = 0x0; keycodes[2] = 0x0;  // Enter keycode, and two null keycodes
    void (**gcallback_functions)() = malloc(sizeof(void*)*10); // Memory management issue
    *gcallback_functions = example_function;                   // The function to be called upon "Enter" press
    struct keyboard_initializer* keyboardi = create_initializer(keybuffer,
                                                                1,
                                                                keycodes,
                                                                gcallback_functions,
                                                                0x0);
    init_keyboard(keyboardi);
 * @endcode
 *
 * @author     Valerie Whitmire
 * @date       2023
 */
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

// Private function definitions
static int attempt_key_callbacks();
static void default_keyboard_callback(registers_t *regs);
static void reset_keyboard();

/**
 * @brief   The keybuffer where all keypresses are stored
 * @note       The address changes whenever a new keyboard initializer is registered.
 * @ingroup KEYBOARD
 */
char* key_buffer = NULL;
struct key_callback *key_callbacks = NULL;
uint8_t c_key = NULL;
uint32_t keypresses = 0;
uint8_t special_key_behavior = 0;

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

struct keyboard_initializer *initializer = NULL;

/**
 * @brief      Creates a keyboard initializer.
 * @ingroup    KEYBOARD
 *
 * @param[in]  n_callbacks           The number of callbacks
 * @param      callbacks_k           The callbacks keycodes
 * @param[in]  callbacks_f           The callbacks functions
 * @param[in]  special_key_behavior  The special key behavior
 * @param      keybuffer_addr        The keybuffer address
 *
 * @return     A pointer to the created keyboard initializer.
 */
struct keyboard_initializer *create_initializer(uint8_t n_callbacks,
                                                uint8_t callbacks_k[],
                                                vf_ptr callbacks_f[],
                                                void* special_key_behavior,
                                                void* keybuffer_addr) {
    struct keyboard_initializer *returnvalue = malloc(sizeof(struct keyboard_initializer));
    int i = 0;
    for(; i < n_callbacks*3; i++)    returnvalue->callback_keycodes[i]  = callbacks_k[i];
    for(; i < 30; i++)               returnvalue->callback_keycodes[i]  = 0x0;
    for(i = 0; i < n_callbacks; i++) returnvalue->callback_functions[i] = callbacks_f[i];

    returnvalue->nkey_buffer      = keybuffer_addr;
    returnvalue->num_callbacks    = n_callbacks;
    returnvalue->general_callback = special_key_behavior;

    return returnvalue;
}

/**
 * @brief      Initializes the keyboard.
 * @ingroup    KEYBOARD
 * @param      nkey_initializer The keyboard initializer
 * @note       The responsibility to free the keybuffer is on the program.
 */
void init_keyboard(struct keyboard_initializer* nkey_initializer) {
    if(initializer != NULL) initializer = free(initializer);
    initializer = nkey_initializer;
    reset_keyboard();
    
    key_buffer = nkey_initializer->nkey_buffer;

    int i = 0;
    for(; i < nkey_initializer->num_callbacks; i++) {
        key_callbacks[i].key_1 = nkey_initializer->callback_keycodes[i*3];
        key_callbacks[i].key_2 = nkey_initializer->callback_keycodes[i*3+1];
        key_callbacks[i].key_3 = nkey_initializer->callback_keycodes[i*3+2];
        key_callbacks[i].callback = nkey_initializer->callback_functions[i];
    }

    if(nkey_initializer->general_callback == (void*)0x0)  {
        register_interrupt_handler(IRQ1, default_keyboard_callback);
        special_key_behavior = 0;
    } else if(nkey_initializer->general_callback == (void*)0x1) {
        register_interrupt_handler(IRQ1, default_keyboard_callback);
        special_key_behavior = 1;
    } else {
        register_interrupt_handler(IRQ1, nkey_initializer->general_callback);
        special_key_behavior = 0;
    }
}

static int attempt_key_callbacks() {
    int i = 0;
    int found_callback = 0;
    for(; i < 10; i++) {
        if(key_callbacks[i].callback == 0x0) break;
        if(keys_pressed[key_callbacks[i].key_1] &&
           (keys_pressed[key_callbacks[i].key_2] || key_callbacks[i].key_2 == 0x0) &&
           (keys_pressed[key_callbacks[i].key_3] || key_callbacks[i].key_3 == 0x0)) {
            (key_callbacks[i].callback)();
            found_callback = 1;
            break;
        }
    }

    return found_callback;
}

static void default_keyboard_callback(registers_t *regs) { 
    keypresses++;
    uint8_t scancode = port_byte_in(0x60);
    c_key = scancode;

    if(scancode < 0x81) keys_pressed[scancode] = 1;
    else                keys_pressed[scancode-0x80] = 0;

    int found_callback = attempt_key_callbacks();
    c_key = (char) found_callback;
    if(!found_callback && scancode < 0x81) {
        if (scancode == BACKSPACE) {
            backspace(key_buffer);
            kprint_backspace();
        } else if (scancode == LSHIFTP) {
        } else if (scancode == RSHIFTP) {
        } else if (scancode == 0x1C && special_key_behavior == 1) {
            append(key_buffer, 0x1C); 
        } else if (scancode == 0x1C && special_key_behavior == 0) {
            append(key_buffer, '\n');
            kprint("\n"); 
        } else {                                                    
            char letter = (keys_pressed[0x2A] || keys_pressed[0x36]) ? ascii_shift[(int) scancode] : ascii[(int) scancode];
            char str[2] = {letter, '\0'}; 
            append(key_buffer, letter); 
            kprint(str);
        }
    }

    UNUSED(regs);
}

static void reset_keyboard() {
    if(key_callbacks != NULL) free(key_callbacks);
    key_callbacks = malloc(sizeof(struct key_callback)*10);
    key_buffer = 0x0;
    int i = 0;
    for(; i < 10; i++) {
        key_callbacks[i].key_1    = 0x0;
        key_callbacks[i].key_2    = 0x0;
        key_callbacks[i].key_3    = 0x0;
        key_callbacks[i].callback = 0x0;
    }
}

/**
 * @brief      Reads a line.
 * @ingroup    KEYBOARD
 * @return     The line which has been read. <i>Must be freed</i>
 */
char* read_line() {
    struct keyboard_initializer *old_initializer = malloc(sizeof(struct keyboard_initializer));
    memory_copy((uint8_t*)initializer, (uint8_t*)old_initializer, sizeof(struct keyboard_initializer));

    char *line_keybuffer = malloc(sizeof(char)*256);
    void (*gcallback_functions[])() = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    uint8_t keycodes[] = {0};
    struct keyboard_initializer* keyboardi = create_initializer(0,
                                                                keycodes,
                                                                gcallback_functions,
                                                                (void*)0x1,
                                                                line_keybuffer);
    init_keyboard(keyboardi);
    while(1) {
        if(character_exists(0x1C, line_keybuffer) > -1) break;
    }
    append(line_keybuffer, '\0');
    char * return_value = malloc(sizeof(char) * strlen(line_keybuffer));
    memory_copy((uint8_t*)line_keybuffer, (uint8_t*)return_value, sizeof(char) * strlen(line_keybuffer));

    free(line_keybuffer);
    init_keyboard(old_initializer);
    return_value[strlen(return_value)-1] = '\0';
    return return_value;
}

/**
 * @brief      Returns the active keybuffer
 * @ingroup    KEYBOARD
 * @return     The keybuffer.
 */
char* get_keybuffer() {
	return key_buffer;
}

/**
 * @brief      While hang until the user presses a key
 * @ingroup    KEYBOARD
 * @todo       Verify function works
 */
void await_keypress() {
    struct keyboard_initializer *old_initializer = malloc(sizeof(struct keyboard_initializer));
    memory_copy((uint8_t*)initializer, (uint8_t*)old_initializer, sizeof(struct keyboard_initializer));

    char *line_keybuffer = malloc(sizeof(char)*256);
    uint8_t keycodes[] = {0x0, 0x0, 0x0};
    void (*gcallback_functions[])() = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    struct keyboard_initializer* keyboardi = create_initializer(0,
                                                                keycodes,
                                                                gcallback_functions,
                                                                (void*)0x1,
                                                                line_keybuffer);
    init_keyboard(keyboardi);
    while(1) if(strlen(line_keybuffer) > 0) break;
    free(line_keybuffer);
    init_keyboard(old_initializer);
}