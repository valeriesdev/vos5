#ifndef POPUP_H
#define POPUP_H

char* create_popup(uint8_t type, void* callback);
struct popup_msg_struct {
	int x1;
	int x2;
	int y1;
	int y2;
	int str_size;
	char* message;
};

struct popup_str_struct {
	int x1;
	int x2;
	int y1;
	int y2;
	int str_size;
	char* message;
	int ret_str_size;
	char** ret_str;
};

#endif