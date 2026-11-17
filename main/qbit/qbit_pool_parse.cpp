#include "qbit_pool_parse.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "ArduinoJson.h"

namespace qbitpool {
namespace {

// Decimal strings ("1.9044") or plain numbers; finite and not negative.
bool decimal(JsonVariantConst v, double *out)
{
    double d;
    if (v.is<const char *>()) {
        const char *s = v.as<const char *>();
        char *end = nullptr;
        d = strtod(s, &end);
        if (end == s || *end != '\0') return false;
    } else if (v.is<double>()) {
        d = v.as<double>();
    } else {
        return false;
    }
    if (!std::isfinite(d) || d < 0) return false;
    *out = d;
    return true;
}

bool count(JsonVariantConst v, int64_t *out)
{
    if (!v.is<int64_t>()) return false;
    int64_t n = v.as<int64_t>();
    if (n < 0) return false;
    *out = n;
    return true;
}

bool count32(JsonVariantConst v, uint32_t *out)
{
    int64_t n;
    if (!count(v, &n) || n > 0xffffffffLL) return false;
    *out = (uint32_t) n;
    return true;
}

} // namespace

bool parse_pool_summary(const char *body, size_t len, PoolSummary *out)
{
    JsonDocument filter;
    filter["pool"]["hashrate_ths"]["h1"] = true;
    filter["pool"]["participants_3h"] = true;
    filter["pool"]["expected_time_to_block_seconds"] = true;
    filter["pool"]["latest_block"]["height"] = true;
    filter["pool"]["latest_block"]["age_seconds"] = true;
    JsonDocument doc;
    if (deserializeJson(doc, body, len, DeserializationOption::Filter(filter))) return false;

    JsonVariantConst pool = doc["pool"];
    PoolSummary s;
    if (!decimal(pool["hashrate_ths"]["h1"], &s.hashrateThs)) return false;
    if (!count32(pool["participants_3h"], &s.participants3h)) return false;
    if (!count32(pool["expected_time_to_block_seconds"], &s.expectedBlockSeconds)) return false;
    if (!count32(pool["latest_block"]["height"], &s.latestBlockHeight)) return false;
    if (!count32(pool["latest_block"]["age_seconds"], &s.latestBlockAgeSeconds)) return false;
    *out = s;
    return true;
}

bool parse_miner(const char *body, size_t len, const char *worker_name, MinerStats *out)
{
    JsonDocument filter;
    filter["reward_window_percent"] = true;
    filter["estimated_next_block"]["estimated_reward_bits"] = true;
    filter["pending_maturity_bits"] = true;
    filter["unpaid_earnings_bits"] = true;
    filter["lifetime_earnings_bits"] = true;
    filter["hashrate_ths"]["m10"] = true;
    filter["workers"][0]["worker_name"] = true;
    filter["workers"][0]["hashrate_ths_60s"] = true;
    filter["recent_payouts"][0]["block_height"] = true;
    filter["recent_payouts"][0]["onchain_amount_bits"] = true;
    JsonDocument doc;
    if (deserializeJson(doc, body, len, DeserializationOption::Filter(filter))) return false;

    MinerStats m = {};
    if (!decimal(doc["reward_window_percent"], &m.windowPercent)) return false;
    if (!count(doc["estimated_next_block"]["estimated_reward_bits"], &m.nextBlockRewardBits)) return false;
    if (!count(doc["pending_maturity_bits"], &m.pendingBits)) return false;
    if (!count(doc["unpaid_earnings_bits"], &m.unpaidBits)) return false;
    if (!count(doc["lifetime_earnings_bits"], &m.lifetimeBits)) return false;
    if (!decimal(doc["hashrate_ths"]["m10"], &m.hashrateThs)) return false;

    if (worker_name && worker_name[0]) {
        for (JsonVariantConst w : doc["workers"].as<JsonArrayConst>()) {
            const char *name = w["worker_name"].as<const char *>();
            if (name && strcmp(name, worker_name) == 0) {
                m.workerFound = decimal(w["hashrate_ths_60s"], &m.workerHashrateThs);
                break;
            }
        }
    }

    JsonVariantConst last = doc["recent_payouts"][0];
    if (!last.isNull()) {
        m.hasLastPayout = count32(last["block_height"], &m.lastPayoutHeight) &&
                          count(last["onchain_amount_bits"], &m.lastPayoutBits);
    }
    *out = m;
    return true;
}

} // namespace qbitpool
