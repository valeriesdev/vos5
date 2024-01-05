#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void init_timer(uint32_t freq);
void wait_ticks(uint32_t n_ticks);

#endif
