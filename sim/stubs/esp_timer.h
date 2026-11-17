#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Microseconds on the simulator's clock (sim/src/esp_stubs.cpp).
int64_t esp_timer_get_time(void);
#ifdef __cplusplus
}
#endif
