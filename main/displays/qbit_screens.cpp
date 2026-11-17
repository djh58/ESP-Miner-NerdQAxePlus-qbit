#include "qbit_screens.h"

#include <cstdio>

#include "qbit_chain.h"

LV_FONT_DECLARE(ui_font_OpenSansBold13);

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
