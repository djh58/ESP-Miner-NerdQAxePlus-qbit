#pragma once
#include <stdint.h>

// The simulator runs on a virtual clock so screenshots are reproducible: LVGL
// ticks and esp_timer_get_time() both advance only through this call.
void sim_clock_advance_ms(uint32_t ms);
