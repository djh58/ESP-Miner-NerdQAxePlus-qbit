#pragma once
#include <stdint.h>
#include <string>

#include "network_stats.h"

// Everything the screens show, as the firmware's modules would report it.
// Field names follow /api/system/info where there is an equivalent.
struct SimScenario {
    std::string name = "default";

    // Mining screen
    float hashrateGhs = 0;
    float powerW = 0;
    float chipTempC = 0;
    int fanRpm = 0;
    int currentMa = 0;
    int vinMv = 0;
    int vcoreMv = 0;
    int64_t uptimeSeconds = 0;
    int64_t sharesAccepted = 0;
    int64_t sharesRejected = 0;
    std::string bestDiff = "0";
    std::string ip = "0.0.0.0";

    // Settings screen (fallback pool mode, the default)
    std::string poolHost;
    int poolPort = 0;
    int frequencyMhz = 0;
    int asicVoltageMv = 0;
    int fanMode = 0; // 0 manual, 1 legacy auto, 2 PID
    int fanSpeedPercent = 0;

    // Price and Global Stats screens: what the explorer fetcher would report.
    // In the JSON a null or missing value is unknown.
    NetworkStats network;
};

// Loads a scenario JSON file. Missing fields keep their defaults.
bool load_scenario(const std::string &path, SimScenario &out, std::string &error);
