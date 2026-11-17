#include "esp_timer.h"
#include "sim_clock.h"

static int64_t s_now_us = 0;

void sim_clock_advance_ms(uint32_t ms) { s_now_us += (int64_t) ms * 1000; }

extern "C" int64_t esp_timer_get_time(void) { return s_now_us; }
