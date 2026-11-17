#pragma once
#include "network_stats.h"
#include "ui.h"

// The qbit edition's screen changes. UI declares this class a friend so it can
// add widgets to and fill the existing screens; it has no ESP-IDF dependency,
// so the desktop simulator (sim/) runs the same code as the firmware.
class QbitScreens {
  public:
    // Adds the widgets the qbit layout of the Global Stats screen needs. Call
    // once, after UI::globalStatsScreenInit().
    static void initGlobalStats(UI &ui);

    // Fills the Global Stats screen. Bitcoin explorers keep the upstream
    // layout; for qbit it shows the permissionless lane with fitting units and
    // the reward step-down in place of the halving countdown. Unknown or stale
    // readings show as "--".
    static void fillGlobalStats(UI &ui, const NetworkStats &stats);

    // The price screen: "--" while the price is unknown.
    static void fillPrice(UI &ui, const NetworkStats &stats);
};
