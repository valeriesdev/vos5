/**
 * @defgroup   POPUP popup
 *
 * @brief      This file implements the various popup routines.
 *
 * The two popup types are
 * - 0: popup that prints a message and is dismissed with a keypress
 * - 1: popup that prints a message and returns the user's input
 * 
 * @todo       If the popup dimensions are all inputted as -1, they will be automatically generated
 *
 * @author     Valerie Whitmire
 * @date       2023
 */
#include <stdint.h>
#include <stddef.h>
#include "drivers/screen.h"
#include "drivers/keyboard.h"
#include "stock/program_interface/popup.h"
#include "libc/string.h"

// Private function definitions
static char* create_popup_msg(struct popup_msg_struct* callback);
static char* create_popup_str(struct popup_str_struct* callback);

static char* create_popup_msg(struct popup_msg_struct* callback) {
	int i = 0;
	for(i = 0; i < callback->x2 - callback->x1; ++i) kprint_at("-", callback->x1 + i, callback->y1);
	for(i = 0; i < callback->x2 - callback->x1; ++i) kprint_at("-", callback->x1 + i, callback->y2);
	for(i = 0; i < callback->y2 - callback->y1; ++i) kprint_at("|", callback->x1, callback->y1 + i);
	for(i = 0; i < callback->y2 - callback->y1; ++i) kprint_at("|", callback->x2, callback->y1 + i);

	kprint_at(callback->message, callback->x1+1, (callback->y1+callback->y2)/2);
	
	await_keypress();

	return NULL;
}

static char* create_popup_str(struct popup_str_struct* callback) {
	int i = 0;
	for(i = 0; i < callback->x2 - callback->x1; ++i) kprint_at("-", callback->x1 + i, callback->y1);
	for(i = 0; i < callback->x2 - callback->x1; ++i) kprint_at("-", callback->x1 + i, callback->y2);
	for(i = 0; i < callback->y2 - callback->y1; ++i) kprint_at("|", callback->x1, callback->y1 + i);
	for(i = 0; i < callback->y2 - callback->y1; ++i) kprint_at("|", callback->x2, callback->y1 + i);

	kprint_at(callback->message, callback->x1+1, (callback->y1+callback->y2)/2);
	char* z = read_line();
	return z;
}

/**
 * void create_popup
 * uint8_t type: 
 * 		0: popup with message
 * 		1: popup that returns a string
 * type, callback
 * 0     points to a struct of int, int, int, int, int,      char[]
 * 							   x1,  x2,  y1,  y2,  str_size, message
 * 
 * 1     points to a struct of int, int, int, int, int     , char[] , int         , char* 
 * 							   x1 , x2 , y1 , y2 , str_size, message, ret_str_size, ret_str
 * */
char* create_popup(uint8_t type, void* callback) {
	char* user_return_value;

	if(type == 0)	   user_return_value = create_popup_msg(callback);
	else if(type == 1) user_return_value = create_popup_str(callback);
	else               return NULL;

	clear_screen();

	return user_return_value;
}