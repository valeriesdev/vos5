/**
 * @defgroup   TIMER timer
 * @ingroup    CPU
 * @brief      This file implements the timer.
 *
 * @author     Valerie Whitmire
 * @date       2023
 */
#include "cpu/timer.h"
#include "cpu/isr.h"
#include "cpu/ports.h"
#include "libc/function.h"

// debugging
#include "drivers/screen.h"
#include "libc/string.h"
#include "libc/mem.h"

volatile uint32_t tick = 0;

/**
 * @brief      The callback to be called on timer IRQs
 *
 * @param      regs  The registers state
 */
static void timer_callback(registers_t *regs) {
    tick++;

    if(tick%25 == 0) {
        char* string = hex_to_ascii((int)get_top());
        kprint_at_preserve(string,0,0);
        string = free(string);
    }


    UNUSED(regs);
}

/**
 * @brief      Initializes the timer.
 *
 * @param[in]  freq  The frequency of the timer
 */
void init_timer(uint32_t freq) {
    /* Install the function we just wrote */
    register_interrupt_handler(IRQ0, timer_callback);

    /* Get the PIT value: hardware clock at 1193180 Hz */
    uint32_t divisor = 1193180 / freq;
    uint8_t low  = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)( (divisor >> 8) & 0xFF);
    /* Send the command */
    port_byte_out(0x43, 0x36); /* Command port */
    port_byte_out(0x40, low);
    port_byte_out(0x40, high);
}

void wait_ticks(uint32_t n_ticks) {
    uint32_t s_tick = tick;
    while(tick-s_tick < n_ticks) {
        //kprint(int_to_ascii(n_ticks));
        //kprint(" ");
        //kprintn(int_to_ascii(tick-s_tick));
    }
}