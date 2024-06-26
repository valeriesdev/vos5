/**
 * @defgroup   SCREEN screen
 * @ingroup    DRIVERS
 * @brief      This file implements a screen driver.
 * 
 * @todo       Implement stock features such as drawing boxes.
 * 
 * @author     Valerie Whitmire
 * @date       2023
 */

#include <stdint.h>
#include "drivers/screen.h"
#include "cpu/ports.h"
#include "libc/mem.h"
#include "kernel/windows.h"
#include "cpu/task_manager.h"

// Private function declarations
static int get_cursor_offset();
static void set_cursor_offset(int offset);
static int print_char(char c, int col, int row, char attr);
static int get_offset(int col, int row);
static int get_offset_row(int offset);
static int get_offset_col(int offset);

extern uint8_t is_alternate_process_running;

static uint32_t *blocked_write_locations = NULL;

void clear_bwl() {
    if(blocked_write_locations != NULL) ta_free (blocked_write_locations);
    blocked_write_locations = NULL;
    blocked_write_locations = ta_alloc(sizeof(uint8_t)*1);
}

void add_bwl(uint32_t new) {
    uint32_t *current = blocked_write_locations;
    uint32_t size = 0;
    while(*current != 0) {
        current++;
        size++;
    }

    uint32_t *new_bwl = ta_alloc(size+1);
    int i = 0;
    for(i = 0; i < size; i++) {
        new_bwl[i] = blocked_write_locations[i];
    }

    ta_free(blocked_write_locations);
    blocked_write_locations = new_bwl;

    if(new != 0)
        new_bwl[size] = new;
    else
        new_bwl[size] = get_cursor_offset();
}

/**
 * @brief      Prints a message at the specified location
 * @ingroup    SCREEN
 * @param      message  The message
 * @param[in]  col      The column
 * @param[in]  row      The row
 */
void kprint_at(char *message, int col, int row) {
    asm volatile("cli");
    /* Set cursor if col/row are negative */
    int offset;
    if (col >= 0 && row >= 0)
        offset = get_offset(col, row);
    else {
        offset = get_cursor_offset();
        row = get_offset_row(offset);
        col = get_offset_col(offset);
    }

    /* Loop through message and print it */
    int i = 0;
    while (message[i] != 0) {
        offset = print_char(message[i++], col, row, WHITE_ON_BLACK);
        /* Compute row/col for next iteration */
        row = get_offset_row(offset);
        col = get_offset_col(offset);
    }
    asm volatile("sti");
}

/**
 * @brief      Print a message at the specified location, preserving cursor position
 * @ingroup    SCREEN
 * @param      message  The message
 * @param[in]  col      The col
 * @param[in]  row      The row
 */
void kprint_at_preserve(char *message, int col, int row) {
    asm volatile("cli");
    int offset = get_cursor_offset();
    kprint_at(message, col, row);
    set_cursor_offset(offset);
    asm volatile("sti");
}

/**
 * @brief      Print a message, with a newline
 * @ingroup    SCREEN
 * @param      message  The message
 */
void kprintn(char *message) {
    kprint(message);
    kprint("\n");
}

/**
 * @brief      Print a message
 * @ingroup    SCREEN
 * @param      message  The message
 */
void kprint(char *message) {
    kprint_at(message, -1, -1);
}

/**
 * @brief      Print a backspace
 * @ingroup    SCREEN
 */
void kprint_backspace() {
    asm volatile("cli");
    uint32_t *current = blocked_write_locations;
    while(*current != 0) {
        if(get_cursor_offset() == *current) {
            return;
        }
        current++;
    }
    int offset = get_cursor_offset()-2;
    int row = get_offset_row(offset);
    int col = get_offset_col(offset);
    print_char(0x08, col, row, WHITE_ON_BLACK);
    asm volatile("sti");
}

/**
 * @brief      Gets the video memory.
 * @ingroup    SCREEN
 */
void* get_video_memory() {
    return (uint8_t*) VIDEO_ADDRESS;
}

/**
 * Innermost print function for our kernel, directly accesses the video memory 
 *
 * If 'col' and 'row' are negative, we will print at current cursor location
 * If 'attr' is zero it will use 'white on black' as default
 * Returns the offset of the next character
 * Sets the video cursor to the returned offset
 */
static int print_char(char c, int col, int row, char attr) {
    uint8_t *vidmem = (uint8_t*) VIDEO_ADDRESS;
    if (!attr) attr = WHITE_ON_BLACK;

    /* Error control: print a red 'E' if the coords aren't right */
    if (col >= MAX_COLS || row >= 50) {
        vidmem[2*(MAX_COLS)*(50)-2] = 'E';
        vidmem[2*(MAX_COLS)*(50)-1] = RED_ON_WHITE;
        return get_offset(col, row);
    }

    int offset;
    if (col >= 0 && row >= 0) offset = get_offset(col, row);
    else offset = get_cursor_offset();

    if (c == '\n') {
        row = get_offset_row(offset);
        offset = get_offset(0, row+1);
    } else if (c == 0x08) { /* Backspace */
        vidmem[offset] = ' ';
        vidmem[offset+1] = attr;
    } else {
        vidmem[offset] = c;
        vidmem[offset+1] = attr;
        offset += 2;
    }


    /* Check if the offset is over screen size and scroll */
    if (offset >= MAX_ROWS * MAX_COLS * 2) {
        int i;
        for (i = 1; i < MAX_ROWS; i++) 
            memory_copy((uint8_t*)(get_offset(0, i) + VIDEO_ADDRESS),
                        (uint8_t*)(get_offset(0, i-1) + VIDEO_ADDRESS),
                        MAX_COLS * 2);

        /* Blank last line */
        char *last_line = (char*) (get_offset(0, MAX_ROWS-1) + (uint8_t*) VIDEO_ADDRESS);
        for (i = 0; i < MAX_COLS * 2; i++) last_line[i] = 0;

        offset -= 2 * MAX_COLS;
        //setup_windows();
    }

    set_cursor_offset(offset);
    return offset;
}

static int get_cursor_offset() {
    /* Use the VGA ports to get the current cursor position
     * 1. Ask for high byte of the cursor offset (data 14)
     * 2. Ask for low byte (data 15)
     */
    port_byte_out(REG_SCREEN_CTRL, 14);
    int offset = port_byte_in(REG_SCREEN_DATA) << 8; /* High byte: << 8 */
    port_byte_out(REG_SCREEN_CTRL, 15);
    offset += port_byte_in(REG_SCREEN_DATA);
    return offset * 2; /* Position * size of character cell */
}

static void set_cursor_offset(int offset) {
    /* Similar to get_cursor_offset, but instead of reading we write data */
    offset /= 2;
    port_byte_out(REG_SCREEN_CTRL, 14);
    port_byte_out(REG_SCREEN_DATA, (uint8_t)(offset >> 8));
    port_byte_out(REG_SCREEN_CTRL, 15);
    port_byte_out(REG_SCREEN_DATA, (uint8_t)(offset & 0xff));
}

void clear_screen() {
    int screen_size = MAX_COLS * 24;
    int i;
    uint8_t *screen = (uint8_t*) VIDEO_ADDRESS;

    for (i = 0; i < screen_size; i++) {
        int j = 0;
        //if(is_alternate_process_running) j = 26*80*2;
        screen[i*2+j] = ' ';
        screen[i*2+1+j] = WHITE_ON_BLACK;
    }
    set_cursor_offset(get_offset(0, 0));
}

static int get_offset(int col, int row) { return 2 * (row * MAX_COLS + col); }
static int get_offset_row(int offset) { return offset / (2 * MAX_COLS); }
static int get_offset_col(int offset) { return (offset - (get_offset_row(offset)*2*MAX_COLS))/2; }