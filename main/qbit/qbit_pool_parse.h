#pragma once
#include <stddef.h>
#include <stdint.h>

// Parsers for the qbit pool's public dashboard API (/public/v1). Amounts are
// in base units ("bits", 1 QBT = 100,000,000); hashrates arrive as decimal
// strings in TH/s. A required field that is missing or malformed fails the
// parse so the screen shows the value as unknown rather than zero.
namespace qbitpool {

// GET /pool-summary
struct PoolSummary {
    double hashrateThs;             // pool.hashrate_ths.h1
    uint32_t participants3h;        // pool.participants_3h
    uint32_t expectedBlockSeconds;  // pool.expected_time_to_block_seconds
    uint32_t latestBlockHeight;     // pool.latest_block.height
    uint32_t latestBlockAgeSeconds; // pool.latest_block.age_seconds, at fetch time
};
bool parse_pool_summary(const char *body, size_t len, PoolSummary *out);

// GET /miners/{payout address}
struct MinerStats {
    double windowPercent;        // reward_window_percent: share of the current reward window
    int64_t nextBlockRewardBits; // estimated_next_block.estimated_reward_bits
    int64_t pendingBits;         // pending_maturity_bits: mined, awaiting coinbase maturity
    int64_t unpaidBits;          // unpaid_earnings_bits: below the payout minimum
    int64_t lifetimeBits;        // lifetime_earnings_bits
    double hashrateThs;          // hashrate_ths.m10, every worker on this address

    bool workerFound;            // a worker with the requested name reported
    double workerHashrateThs;    // that worker's hashrate_ths_60s

    bool hasLastPayout;          // recent_payouts[0]
    uint32_t lastPayoutHeight;
    int64_t lastPayoutBits;      // onchain_amount_bits
};
// `worker_name` may be null or empty when the username has no worker part.
bool parse_miner(const char *body, size_t len, const char *worker_name, MinerStats *out);

} // namespace qbitpool
