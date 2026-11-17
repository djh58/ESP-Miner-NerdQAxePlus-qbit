// Puts scenario values into the screens. For the upstream screens this mirrors
// DisplayDriver's update functions in main/displays/displayDriver.cpp; keep the
// formats in step when those change. The qbit edition's screens are filled by
// the firmware's own QbitScreens code.
#include <cstdio>
#include <cstring>

#include "displayDriver.h"
#include "qbit_screens.h"
#include "sim_scenario.h"

namespace {

void formatHashrate(char *buf, int len, float hashrate)
{
    if (hashrate >= 10000.0) {
        snprintf(buf, len, "%d", (int) (hashrate + 0.5f));
    } else {
        snprintf(buf, len, "%.1f", hashrate);
    }
}

void setText(lv_obj_t *label, const char *text)
{
    if (label) lv_label_set_text(label, text);
}

} // namespace

void DisplayDriver::fill(UI &ui, const SimScenario &s)
{
    char str[32];

    // updateGlobalState
    snprintf(str, sizeof(str), "%.0f", s.chipTempC);
    setText(ui.ui_lbTemp, str);
    setText(ui.ui_lblTempPrice, str);
    snprintf(str, sizeof(str), "%d", s.fanRpm);
    setText(ui.ui_lbRPM, str);
    snprintf(str, sizeof(str), "%imA", s.currentMa);
    setText(ui.ui_lbIntensidad, str);
    snprintf(str, sizeof(str), "%imV", s.vinMv);
    setText(ui.ui_lbVinput, str);
    snprintf(str, sizeof(str), "%umV", (unsigned) s.vcoreMv);
    setText(ui.ui_lbVcore, str);

    // updateTime
    int64_t up = s.uptimeSeconds;
    snprintf(str, sizeof(str), "%dd %ih %im %is", (int) (up / 86400), (int) (up % 86400 / 3600),
             (int) (up % 3600 / 60), (int) (up % 60));
    setText(ui.ui_lbTime, str);

    // updateShares (fallback pool mode)
    snprintf(str, sizeof(str), "%lld/%lld", (long long) s.sharesAccepted, (long long) s.sharesRejected);
    setText(ui.ui_lbShares, str);
    setText(ui.ui_lbBestDifficulty, s.bestDiff.c_str());
    setText(ui.ui_lbBestDifficultySet, s.bestDiff.c_str());

    // updateHashrate
    formatHashrate(str, sizeof(str), s.hashrateGhs);
    setText(ui.ui_lbHashrate, str);
    setText(ui.ui_lbHashrateSet, str);
    setText(ui.ui_lblHashPrice, str);
    float hr = s.hashrateGhs / 1000.0f;
    float efficiency = (hr > 0) ? s.powerW / hr : 10000.0f;
    snprintf(str, sizeof(str), "%.1f", efficiency);
    setText(ui.ui_lbEficiency, (efficiency < 10000.0f) ? str : "n/a");
    snprintf(str, sizeof(str), "%.3fW", s.powerW);
    setText(ui.ui_lbPower, str);

    // updateIpAddress
    setText(ui.ui_lbIP, s.ip.c_str());
    setText(ui.ui_lbIPSet, s.ip.c_str());

    // updateCurrentSettings (fallback pool mode)
    setText(ui.ui_lbPoolSet, s.poolHost.c_str());
    snprintf(str, sizeof(str), "%d", s.poolPort);
    setText(ui.ui_lbPortSet, str);
    snprintf(str, sizeof(str), "%d", s.frequencyMhz);
    setText(ui.ui_lbFreqSet, str);
    snprintf(str, sizeof(str), "%d", s.asicVoltageMv);
    setText(ui.ui_lbVcoreSet, str);
    if (s.fanMode == 1) {
        setText(ui.ui_lbFanSet, "AUTO");
    } else if (s.fanMode == 2) {
        setText(ui.ui_lbFanSet, "PID");
    } else {
        snprintf(str, sizeof(str), "%d", s.fanSpeedPercent);
        setText(ui.ui_lbFanSet, str);
    }

    // updateBTCprice and updateGlobalMiningStats share the firmware's code.
    QbitScreens::fillPrice(ui, s.network);
    QbitScreens::fillGlobalStats(ui, s.network);
}

static const char *const kScreens[] = {"splash1", "splash2", "portal", "mining", "settings", "price", "global", nullptr};

const char *const *DisplayDriver::screenNames() { return kScreens; }

lv_obj_t *DisplayDriver::screenByName(UI &ui, const char *name)
{
    if (!strcmp(name, "splash1")) return ui.ui_Splash1;
    if (!strcmp(name, "splash2")) return ui.ui_Splash2;
    if (!strcmp(name, "portal")) return ui.ui_PortalScreen;
    if (!strcmp(name, "mining")) return ui.ui_MiningScreen;
    if (!strcmp(name, "settings")) return ui.ui_SettingsScreen;
    if (!strcmp(name, "price")) return ui.ui_BTCScreen;
    if (!strcmp(name, "global")) return ui.ui_GlobalStats;
    return nullptr;
}
