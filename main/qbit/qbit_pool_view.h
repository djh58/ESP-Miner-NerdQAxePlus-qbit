#pragma once
#include <stdint.h>

#include "qbit_pool_parse.h"

// What the qbit pool screen and block toast show; filled by the pool fetcher
// (main/tasks/qbit_pool_task) or, in the simulator, by a scenario file.
struct QbitPoolView {
    bool poolKnown = false;
    qbitpool::PoolSummary pool = {};
    bool minerKnown = false;
    qbitpool::MinerStats miner = {};
    char address[96] = "";     // payout address the stats are for
    char worker[32] = "";      // this miner's worker name, from the stratum username
    uint32_t sinceFetchSeconds = 0; // time since the readings were fetched
};

// A block the pool found since the previous reading.
struct QbitPoolBlock {
    uint32_t height = 0;
    bool paid = false;     // the address's payout for this block is known
    int64_t paidBits = 0;
    double sharePercent = 0; // the address's reward window share when known
};
