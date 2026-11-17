#pragma once
#include "network_stats.h"
#include "qbit_address.h"
#include "qbit_pool_view.h"
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

    // A red banner on the mining screen while a pool's username is not a
    // payout address the qbit pool will credit (it would credit the pool's
    // fallback address instead). Call once after UI::miningScreenInit().
    static void initPayoutWarning(UI &ui);
    static void setPayoutWarning(UI &ui, const qbit::PayoutCheck &primary, const qbit::PayoutCheck &fallback);

    // The qbit pool screen: your share of the reward window, the reward if the
    // pool's next block lands now, estimated QBT per day, pending and lifetime
    // earnings, and this miner's worker as the pool sees it. Created when
    // shown and deleted when left, so it costs no LVGL memory otherwise.
    static lv_obj_t *createPoolScreen(UI &ui);
    static void deletePoolScreen(UI &ui);
    static void fillPoolScreen(UI &ui, const QbitPoolView &view);

    // A short toast over whatever screen is showing when the pool finds a
    // block (about every two minutes), with what this address earned from it.
    static void showBlockToast(UI &ui, const QbitPoolBlock &block);
    static void hideBlockToast(UI &ui);
};
