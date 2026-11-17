// LVGL configuration for the desktop simulator. The firmware configures LVGL
// through Kconfig (sdkconfig.defaults) rather than main/displays/lv_conf.h, so
// this mirrors those effective values; everything else takes LVGL's defaults,
// as it does on the device.
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1 // CONFIG_LV_COLOR_16_SWAP=y

// The device uses LVGL's built-in pool at the size sdkconfig.defaults sets
// (CONFIG_LV_MEM_SIZE_KILOBYTES), so a screen that runs out of LVGL memory
// here would run out on the miner too. Keep the two in step.
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U)

#define LV_USE_SNAPSHOT 1 // CONFIG_LV_USE_SNAPSHOT=y

#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_14 1

// LVGL's default assert handler spins forever, as it does on the device; the
// simulator stops with a message instead so a memory shortfall is visible.
#define LV_ASSERT_HANDLER_INCLUDE <stdlib.h>
#define LV_ASSERT_HANDLER abort();

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

#endif
