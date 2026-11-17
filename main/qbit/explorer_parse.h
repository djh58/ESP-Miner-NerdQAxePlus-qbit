#pragma once
#include <stddef.h>
#include <stdint.h>

// Parsers for the mempool-style explorer API the stats screens read
// (mempool.space, mempool.qbit.org). They take the raw response body and
// report failure rather than inventing values, so a missing, malformed or
// out-of-range field shows as unknown instead of zero.
namespace explorer {

// GET /api/v1/prices -> {"USD": 64442, ...}. False when USD is missing, not a
// number or not positive (mempool.qbit.org has no price feed and returns -1).
bool parse_price_usd(const char *body, size_t len, uint32_t *usd);

// GET /api/blocks/tip/height -> 976212
bool parse_block_height(const char *body, size_t len, uint32_t *height);

struct NetworkHashrate {
    double hashrate;    // hashes per second
    double difficulty;
    bool qbit_lanes;    // the response carried qbit's per-lane fields
};

// GET /api/v1/mining/hashrate/3d. qbit explorers report two mining lanes; ASIC
// miners like this one mine the permissionless lane, so its hashrate and
// difficulty are used when present (the legacy currentHashrate is the total
// of both lanes while currentDifficulty is the permissionless lane's).
bool parse_network_hashrate(const char *body, size_t len, NetworkHashrate *out);

struct Fees {
    uint32_t hour;
    uint32_t half_hour;
    uint32_t fastest;
};

// GET /api/v1/fees/recommended
bool parse_fees(const char *body, size_t len, Fees *out);

} // namespace explorer
