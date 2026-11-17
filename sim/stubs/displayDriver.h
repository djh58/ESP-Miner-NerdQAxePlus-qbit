// Simulator stand-in for main/displays/displayDriver.h. UI declares
// DisplayDriver a friend, so the simulator's label filling lives here and can
// reach the same private widgets the firmware's driver updates.
#pragma once
#include "ui.h"

#define TDISPLAYS3_LCD_H_RES 320
#define TDISPLAYS3_LCD_V_RES 170

struct SimScenario;

class DisplayDriver {
  public:
    void setScreenAnimationRunning(bool running) { (void) running; }

    // Implemented in sim/src/sim_fill.cpp.
    static void fill(UI &ui, const SimScenario &s);
    static lv_obj_t *screenByName(UI &ui, const char *name);
    static const char *const *screenNames();
};
