#include "qbit_screens.h"

#include <cstdio>
#include <cstring>

#include "qbit_chain.h"

LV_FONT_DECLARE(ui_font_OpenSansBold13);
LV_FONT_DECLARE(ui_font_OpenSansBold14);
LV_FONT_DECLARE(ui_font_OpenSansBold24);

// ui.cpp: tells the display driver a screen animation finished.
void on_screen_loaded(lv_event_t *e);

namespace {

// Pixel positions in the 321x170 Global Stats artwork of the qbit layout's
// replacements for the baked-in "EH/s" and "HALVING" text, and the color of
// the bands they sit on.
constexpr lv_coord_t kUnitMaskX = 286, kUnitMaskY = 111, kUnitMaskW = 31, kUnitMaskH = 14;
constexpr lv_coord_t kStepMaskX = 0, kStepMaskY = 115, kStepMaskW = 67, kStepMaskH = 13;
const uint32_t kBandColor = 0xC5C6C5;

lv_obj_t *mask(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, w, h);
    lv_obj_set_style_bg_color(box, lv_color_hex(kBandColor), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

lv_obj_t *bandLabel(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(label, &ui_font_OpenSansBold13, 0);
    return label;
}

void setHidden(lv_obj_t *obj, bool hidden)
{
    if (!obj) return;
    if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

void setText(lv_obj_t *label, const char *text)
{
    if (label) lv_label_set_text(label, text);
}

} // namespace

void QbitScreens::initGlobalStats(UI &ui)
{
    if (!ui.ui_GlobalStats) return;

    ui.ui_qbitUnitMask = mask(ui.ui_GlobalStats, kUnitMaskX, kUnitMaskY, kUnitMaskW, kUnitMaskH);
    ui.ui_qbitUnit = bandLabel(ui.ui_qbitUnitMask, "PH/s");
    lv_obj_align(ui.ui_qbitUnit, LV_ALIGN_RIGHT_MID, -3, 0);

    ui.ui_qbitStepMask = mask(ui.ui_GlobalStats, kStepMaskX, kStepMaskY, kStepMaskW, kStepMaskH);
    // qbit has no halving: the reward steps down 4.32% every 43,200 blocks.
    lv_obj_t *step = bandLabel(ui.ui_qbitStepMask, "STEPDOWN");
    lv_obj_set_style_text_letter_space(step, -1, 0); // fits the tab like the baked-in "HALVING"
    lv_obj_align(step, LV_ALIGN_LEFT_MID, 2, 0);

    setHidden(ui.ui_qbitUnitMask, true);
    setHidden(ui.ui_qbitStepMask, true);
}

void QbitScreens::fillGlobalStats(UI &ui, const NetworkStats &s)
{
    char str[32];
    setHidden(ui.ui_qbitUnitMask, !s.qbit);
    setHidden(ui.ui_qbitStepMask, !s.qbit);

    if (s.heightKnown) {
        snprintf(str, sizeof(str), "%lu", (unsigned long) s.blockHeight);
        setText(ui.ui_lblBlock, str);
        uint32_t remaining, percent;
        if (s.qbit) {
            remaining = qbit::blocks_to_next_stepdown(s.blockHeight);
            percent = qbit::stepdown_progress_percent(s.blockHeight);
        } else {
            const uint32_t halving = 210000;
            remaining = ((s.blockHeight / halving) + 1) * halving - s.blockHeight;
            percent = (halving - remaining) * 100 / halving;
        }
        snprintf(str, sizeof(str), "%lu", (unsigned long) remaining);
        setText(ui.ui_lblBlocksToHalving, str);
        snprintf(str, sizeof(str), "%lu%%", (unsigned long) percent);
        setText(ui.ui_lblHalvingPercent, str);
    } else {
        setText(ui.ui_lblBlock, "--");
        setText(ui.ui_lblBlocksToHalving, "--");
        setText(ui.ui_lblHalvingPercent, "--");
    }

    if (!s.hashrateKnown) {
        setText(ui.ui_lblGlobalHash, "--");
        setText(ui.ui_lblDifficulty, "--");
    } else if (s.qbit) {
        const char *unit = "";
        qbit::format_hashrate(s.hashrate, str, sizeof(str), &unit);
        setText(ui.ui_lblGlobalHash, str);
        setText(ui.ui_qbitUnit, unit);
        qbit::format_difficulty(s.difficulty, str, sizeof(str));
        setText(ui.ui_lblDifficulty, str);
    } else {
        // Upstream formats: whole EH/s and whole T.
        snprintf(str, sizeof(str), "%llu", (unsigned long long) (s.hashrate / 1e18));
        setText(ui.ui_lblGlobalHash, str);
        snprintf(str, sizeof(str), "%lluT", (unsigned long long) (s.difficulty / 1e12));
        setText(ui.ui_lblDifficulty, str);
    }

    if (s.feesKnown) {
        snprintf(str, sizeof(str), "%lu", (unsigned long) s.feeHour);
        setText(ui.ui_lbllowFee, str);
        snprintf(str, sizeof(str), "%lu", (unsigned long) s.feeHalfHour);
        setText(ui.ui_lblmedFee, str);
        snprintf(str, sizeof(str), "%lu", (unsigned long) s.feeFastest);
        setText(ui.ui_lblhighFee, str);
    } else {
        setText(ui.ui_lbllowFee, "--");
        setText(ui.ui_lblmedFee, "--");
        setText(ui.ui_lblhighFee, "--");
    }
}

void QbitScreens::fillPrice(UI &ui, const NetworkStats &s)
{
    char str[24];
    if (s.priceKnown) {
        snprintf(str, sizeof(str), "%lu$", (unsigned long) s.priceUsd);
        setText(ui.ui_lblBTCPrice, str);
    } else {
        setText(ui.ui_lblBTCPrice, "--");
    }
}

void QbitScreens::initPayoutWarning(UI &ui)
{
    if (!ui.ui_MiningScreen) return;

    // Over the logo, the one decorative area of the mining screen.
    lv_obj_t *banner = lv_obj_create(ui.ui_MiningScreen);
    lv_obj_remove_style_all(banner);
    lv_obj_set_pos(banner, 8, 24);
    lv_obj_set_size(banner, 146, 40);
    lv_obj_set_style_radius(banner, 6, 0);
    lv_obj_set_style_bg_color(banner, lv_color_hex(0xE60000), 0);
    lv_obj_set_style_bg_opa(banner, LV_OPA_COVER, 0);
    lv_obj_clear_flag(banner, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *text = lv_label_create(banner);
    lv_obj_set_style_text_color(text, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(text, &ui_font_OpenSansBold13, 0);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(text);

    ui.ui_qbitPayoutBanner = banner;
    ui.ui_qbitPayoutText = text;
    setHidden(banner, true);
}

void QbitScreens::setPayoutWarning(UI &ui, const qbit::PayoutCheck &primary, const qbit::PayoutCheck &fallback)
{
    auto bad = [](const qbit::PayoutCheck &c) {
        return c.status == qbit::PayoutStatus::Invalid || c.status == qbit::PayoutStatus::NetworkMismatch;
    };
    const qbit::PayoutCheck *problem = bad(primary) ? &primary : bad(fallback) ? &fallback : nullptr;
    setHidden(ui.ui_qbitPayoutBanner, problem == nullptr);
    if (!problem) return;

    const char *what = problem == &primary ? "PAYOUT ADDRESS" : "BACKUP POOL ADDRESS";
    const char *why = problem->status == qbit::PayoutStatus::NetworkMismatch ? "WRONG NETWORK" : "NOT VALID";
    char str[48];
    snprintf(str, sizeof(str), "%s\n%s", what, why);
    setText(ui.ui_qbitPayoutText, str);
    if (ui.ui_qbitPayoutText) lv_obj_center(ui.ui_qbitPayoutText);
}

struct QbitPoolWidgets {
    lv_obj_t *poolStats;
    lv_obj_t *share;
    lv_obj_t *nextBlock;
    lv_obj_t *perDay;
    lv_obj_t *worker;
    lv_obj_t *earnings;
    lv_obj_t *footer;
};

namespace {

const uint32_t kMagenta = 0xE600DE;
const uint32_t kGrey = 0x9A9A9A;

lv_obj_t *poolLabel(lv_obj_t *parent, const lv_font_t *font, uint32_t color, lv_coord_t x, lv_coord_t y,
                    const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_pos(label, x, y);
    lv_label_set_text(label, text);
    return label;
}

// "1853" for big amounts, "9.7" for small ones, from base units.
void formatApproxQbt(double bits, char *out, size_t len)
{
    double qbt = bits / (double) qbit::kCoin;
    snprintf(out, len, qbt >= 100.0 ? "%.0f" : "%.1f", qbt);
}

void formatAge(uint32_t seconds, char *out, size_t len)
{
    if (seconds < 90) snprintf(out, len, "%lus ago", (unsigned long) seconds);
    else if (seconds < 90 * 60) snprintf(out, len, "%lum ago", (unsigned long) (seconds / 60));
    else snprintf(out, len, "%luh ago", (unsigned long) (seconds / 3600));
}

} // namespace

lv_obj_t *QbitScreens::createPoolScreen(UI &ui)
{
    if (ui.ui_qbitPoolScreen) return ui.ui_qbitPoolScreen;

    lv_obj_t *scr = lv_obj_create(nullptr);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, 320, 22);
    lv_obj_set_style_bg_color(header, lv_color_hex(kMagenta), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);

    QbitPoolWidgets *w = new QbitPoolWidgets();
    poolLabel(header, &ui_font_OpenSansBold14, 0x000000, 8, 2, "qbit pool");
    w->poolStats = poolLabel(header, &ui_font_OpenSansBold13, 0x000000, 0, 3, "");
    lv_obj_align(w->poolStats, LV_ALIGN_TOP_RIGHT, -8, 3);

    const char *titles[] = {"YOUR SHARE", "NEXT BLOCK", "PER DAY"};
    const char *units[] = {"of window", "QBT", "QBT est."};
    lv_obj_t **values[] = {&w->share, &w->nextBlock, &w->perDay};
    for (int i = 0; i < 3; i++) {
        lv_coord_t x = 8 + i * 106;
        poolLabel(scr, &ui_font_OpenSansBold13, kGrey, x, 28, titles[i]);
        *values[i] = poolLabel(scr, &ui_font_OpenSansBold24, 0xFFFFFF, x, 42, "--");
        poolLabel(scr, &ui_font_OpenSansBold13, kMagenta, x, 72, units[i]);
    }

    lv_obj_t *rule = lv_obj_create(scr);
    lv_obj_remove_style_all(rule);
    lv_obj_set_pos(rule, 8, 94);
    lv_obj_set_size(rule, 304, 1);
    lv_obj_set_style_bg_color(rule, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);

    w->worker = poolLabel(scr, &ui_font_OpenSansBold13, 0xFFFFFF, 8, 100, "");
    w->earnings = poolLabel(scr, &ui_font_OpenSansBold13, 0xFFFFFF, 8, 120, "");
    w->footer = poolLabel(scr, &ui_font_OpenSansBold13, kGrey, 8, 148, "");
    lv_label_set_recolor(w->worker, true);
    lv_label_set_recolor(w->earnings, true);

    // As for the upstream screens, so the driver knows when the animation ends.
    lv_obj_add_event_cb(scr, on_screen_loaded, LV_EVENT_SCREEN_LOADED, ui.m_display);

    ui.ui_qbitPoolScreen = scr;
    ui.ui_qbitPool = w;
    return scr;
}

void QbitScreens::deletePoolScreen(UI &ui)
{
    lv_obj_t *scr = ui.ui_qbitPoolScreen;
    if (!scr) return;
    // Never delete a screen that is showing or part of a screen animation.
    lv_disp_t *disp = lv_disp_get_default();
    if (lv_scr_act() == scr || (disp && (disp->prev_scr == scr || disp->scr_to_load == scr))) return;
    lv_obj_del(scr);
    delete ui.ui_qbitPool;
    ui.ui_qbitPoolScreen = nullptr;
    ui.ui_qbitPool = nullptr;
}

void QbitScreens::fillPoolScreen(UI &ui, const QbitPoolView &v)
{
    QbitPoolWidgets *w = ui.ui_qbitPool;
    if (!w) return;
    char str[96], a[24], b[24];

    if (v.poolKnown) {
        const char *unit = "";
        qbit::format_hashrate(v.pool.hashrateThs * 1e12, a, sizeof(a), &unit);
        snprintf(str, sizeof(str), "%s %s  %lu miners", a, unit, (unsigned long) v.pool.participants3h);
        setText(w->poolStats, str);
    } else {
        setText(w->poolStats, "--");
    }

    double blocksPerDay = (v.poolKnown && v.pool.expectedBlockSeconds > 0) ? 86400.0 / v.pool.expectedBlockSeconds : 0;
    if (v.minerKnown) {
        snprintf(str, sizeof(str), "%.2f%%", v.miner.windowPercent);
        setText(w->share, str);
        qbit::format_qbt(v.miner.nextBlockRewardBits, str, sizeof(str));
        setText(w->nextBlock, str);
        if (blocksPerDay > 0) {
            formatApproxQbt(v.miner.nextBlockRewardBits * blocksPerDay, str, sizeof(str));
            setText(w->perDay, str);
        } else {
            setText(w->perDay, "--");
        }

        if (!v.worker[0]) {
            setText(w->worker, "#9A9A9A no worker name in the username#");
        } else if (!v.miner.workerFound) {
            snprintf(str, sizeof(str), "#E600DE %s# #FF5050 not seen by the pool#", v.worker);
            setText(w->worker, str);
        } else {
            const char *unit = "";
            qbit::format_hashrate(v.miner.workerHashrateThs * 1e12, a, sizeof(a), &unit);
            // This worker's slice of what the address earns per block.
            double perBlockToMiners = v.miner.windowPercent > 0 ? v.miner.nextBlockRewardBits * 100.0 / v.miner.windowPercent : 0;
            double workerShare = (v.poolKnown && v.pool.hashrateThs > 0) ? v.miner.workerHashrateThs / v.pool.hashrateThs : 0;
            if (blocksPerDay > 0 && perBlockToMiners > 0 && workerShare > 0) {
                formatApproxQbt(perBlockToMiners * workerShare * blocksPerDay, b, sizeof(b));
                snprintf(str, sizeof(str), "#E600DE %s# %s %s at pool, ~%s QBT/day", v.worker, a, unit, b);
            } else {
                snprintf(str, sizeof(str), "#E600DE %s# %s %s at pool", v.worker, a, unit);
            }
            setText(w->worker, str);
        }

        qbit::format_qbt(v.miner.pendingBits, a, sizeof(a));
        qbit::format_qbt(v.miner.lifetimeBits, b, sizeof(b));
        snprintf(str, sizeof(str), "#9A9A9A PENDING# %s  #9A9A9A LIFETIME# %s QBT", a, b);
        setText(w->earnings, str);
    } else {
        setText(w->share, "--");
        setText(w->nextBlock, "--");
        setText(w->perDay, "--");
        setText(w->worker, "");
        setText(w->earnings, v.address[0] ? "#9A9A9A waiting for the pool#" : "#FF5050 set a qbit payout address#");
    }

    // qb1zz2q6..tjuhax, last block #178869 2m ago
    size_t n = strlen(v.address);
    if (n > 16) snprintf(a, sizeof(a), "%.8s..%s", v.address, v.address + n - 6);
    else snprintf(a, sizeof(a), "%s", v.address);
    if (v.poolKnown) {
        formatAge(v.pool.latestBlockAgeSeconds + v.sinceFetchSeconds, b, sizeof(b));
        snprintf(str, sizeof(str), "%s  block #%lu %s", a, (unsigned long) v.pool.latestBlockHeight, b);
    } else {
        snprintf(str, sizeof(str), "%s", a);
    }
    setText(w->footer, str);
}

void QbitScreens::showBlockToast(UI &ui, const QbitPoolBlock &block)
{
    hideBlockToast(ui);
    lv_obj_t *toast = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(toast);
    lv_obj_set_size(toast, 300, 38);
    lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_radius(toast, 8, 0);
    lv_obj_set_style_bg_color(toast, lv_color_hex(kMagenta), 0);
    lv_obj_set_style_bg_opa(toast, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(toast, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(toast, 2, 0);
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    char str[80], amount[24];
    if (block.paid) {
        qbit::format_qbt(block.paidBits, amount, sizeof(amount));
        snprintf(str, sizeof(str), "qbit pool found block #%lu\n+%s QBT to you", (unsigned long) block.height, amount);
    } else if (block.sharePercent > 0) {
        snprintf(str, sizeof(str), "qbit pool found block #%lu\nyour share %.2f%%", (unsigned long) block.height,
                 block.sharePercent);
    } else {
        snprintf(str, sizeof(str), "qbit pool found block #%lu", (unsigned long) block.height);
    }
    lv_obj_t *text = lv_label_create(toast);
    lv_obj_set_style_text_font(text, &ui_font_OpenSansBold13, 0);
    lv_obj_set_style_text_color(text, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(text, str);
    lv_obj_center(text);
    ui.ui_qbitBlockToast = toast;
}

void QbitScreens::hideBlockToast(UI &ui)
{
    if (!ui.ui_qbitBlockToast) return;
    lv_obj_del(ui.ui_qbitBlockToast);
    ui.ui_qbitBlockToast = nullptr;
}
