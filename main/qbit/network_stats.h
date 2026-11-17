#pragma once
#include <stdint.h>

// What the explorer fetcher last learned about the network, for the stats
// screens. A value is shown only while its `known` flag is set: it is cleared
// when the explorer address changes and when the value grows too old, so the
// screens never present another network's or a stale reading as current.
struct NetworkStats {
    bool qbit = false; // the explorer reported qbit's per-lane mining data

    bool priceKnown = false;
    uint32_t priceUsd = 0;

    bool heightKnown = false;
    uint32_t blockHeight = 0;

    // For qbit, the permissionless lane that ASIC miners mine.
    bool hashrateKnown = false;
    double hashrate = 0; // hashes per second
    double difficulty = 0;

    bool feesKnown = false;
    uint32_t feeHour = 0;
    uint32_t feeHalfHour = 0;
    uint32_t feeFastest = 0;
};
