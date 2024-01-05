#ifndef SCREEN_H
#define SCREEN_H

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25 //this is a lie. there are 50 rows but each program only uses at most 25 
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f
#define RED_ON_WHITE 0xf4

/* Screen i/o ports */
#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

#include <stdint.h>

/* Public kernel API */
void clear_screen();
void kprint_at(char *message, int col, int row);
void kprint_at_preserve(char *message, int col, int row);
void kprint(char *message);
void kprintn(char *message);
void kprint_backspace();
void* get_video_memory();
void clear_bwl();
void add_bwl(uint32_t new) ;
void kprint_w(char* message, int window);
#endif