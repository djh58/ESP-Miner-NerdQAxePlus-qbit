#pragma once
#include <stddef.h>
#include <stdint.h>

// qbit consensus and display helpers that have no ESP-IDF dependency, so the
// desktop simulator and host tests use the same code as the firmware.
namespace qbit {

constexpr int64_t kCoin = 100000000; // base units per QBT

// Block reward schedule (qbit src/validation.cpp GetBlockSubsidy with the
// mainnet and testnet chain parameters): 210 QBT, then every 43,200 blocks the
// reward is multiplied by 598/625, a 4.32% step-down. There is no halving.
constexpr uint32_t kSubsidyStepInterval = 43200;
constexpr int64_t kSubsidyInitial = 210 * kCoin;
constexpr int64_t kStepdownNumerator = 598;
constexpr int64_t kStepdownDenominator = 625;

// Block reward in base units at `height`, with consensus integer rounding.
int64_t block_subsidy(uint32_t height);

// Blocks until the next step-down (1..kSubsidyStepInterval).
uint32_t blocks_to_next_stepdown(uint32_t height);

// Whole percent of the current step already mined (0..99).
uint32_t stepdown_progress_percent(uint32_t height);

// "86.4" with unit "PH/s"; picks H/s through EH/s so the value stays below
// 1000. Writes "--" and unit "" for a negative or non-finite rate.
void format_hashrate(double hashes_per_second, char *value, size_t value_len, const char **unit);

// Difficulty with an SI suffix: 1.35G, 20.9T, 134T, 512. "--" when invalid.
void format_difficulty(double difficulty, char *out, size_t out_len);

// Amount in base units as QBT with two decimals, rounded down: "176.00".
void format_qbt(int64_t base_units, char *out, size_t out_len);

} // namespace qbit
