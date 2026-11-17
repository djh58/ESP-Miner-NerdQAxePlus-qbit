// Simulator stand-in for main/boards/board.h: the UI only asks the board for
// its theme and model names.
#pragma once
#include <algorithm>
#include <vector>

#include "themes.h"

// From main/nvs_config.h, which the real board.h includes.
#ifndef CONFIG_DONATE_ADDR
#define CONFIG_DONATE_ADDR "bc1q7n70rumyv6lvu8avpml0c3uggvssfu52egum3q"
#endif

class Board {
  public:
    Board(Theme *theme, const char *deviceModel, const char *asicModel)
        : m_theme(theme), m_deviceModel(deviceModel), m_asicModel(asicModel) {}

    Theme *getTheme() { return m_theme; }
    const char *getDeviceModel() { return m_deviceModel; }
    const char *getAsicModel() { return m_asicModel; }

  private:
    Theme *m_theme;
    const char *m_deviceModel;
    const char *m_asicModel;
};
