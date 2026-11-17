#include "explorer_parse.h"

#include <cmath>

#include "ArduinoJson.h"

namespace explorer {
namespace {

bool positive_finite(JsonVariantConst v, double *out)
{
    if (!v.is<double>()) return false;
    double d = v.as<double>();
    if (!std::isfinite(d) || d <= 0) return false;
    *out = d;
    return true;
}

bool to_u32(JsonVariantConst v, uint32_t *out)
{
    if (!v.is<double>()) return false;
    double d = v.as<double>();
    if (!std::isfinite(d) || d < 0 || d > 4294967295.0 || d != std::floor(d)) return false;
    *out = (uint32_t) d;
    return true;
}

} // namespace

bool parse_price_usd(const char *body, size_t len, uint32_t *usd)
{
    JsonDocument filter;
    filter["USD"] = true;
    JsonDocument doc;
    if (deserializeJson(doc, body, len, DeserializationOption::Filter(filter))) return false;
    double d;
    if (!positive_finite(doc["USD"], &d) || d > 4294967295.0) return false;
    *usd = (uint32_t) std::lround(d);
    return true;
}

bool parse_block_height(const char *body, size_t len, uint32_t *height)
{
    JsonDocument doc;
    if (deserializeJson(doc, body, len)) return false;
    return to_u32(doc.as<JsonVariantConst>(), height) && *height > 0;
}

bool parse_network_hashrate(const char *body, size_t len, NetworkHashrate *out)
{
    JsonDocument filter;
    filter["currentHashrate"] = true;
    filter["currentDifficulty"] = true;
    filter["currentHashratePermissionless"] = true;
    filter["currentDifficultyPermissionless"] = true;
    JsonDocument doc;
    if (deserializeJson(doc, body, len, DeserializationOption::Filter(filter))) return false;

    NetworkHashrate r = {};
    r.qbit_lanes = !doc["currentHashratePermissionless"].isNull() || !doc["currentDifficultyPermissionless"].isNull();
    const char *hash_key = r.qbit_lanes ? "currentHashratePermissionless" : "currentHashrate";
    const char *diff_key = r.qbit_lanes ? "currentDifficultyPermissionless" : "currentDifficulty";
    if (!positive_finite(doc[hash_key], &r.hashrate)) return false;
    if (!positive_finite(doc[diff_key], &r.difficulty)) return false;
    *out = r;
    return true;
}

bool parse_fees(const char *body, size_t len, Fees *out)
{
    JsonDocument filter;
    filter["hourFee"] = true;
    filter["halfHourFee"] = true;
    filter["fastestFee"] = true;
    JsonDocument doc;
    if (deserializeJson(doc, body, len, DeserializationOption::Filter(filter))) return false;
    Fees f;
    if (!to_u32(doc["hourFee"], &f.hour) || !to_u32(doc["halfHourFee"], &f.half_hour) ||
        !to_u32(doc["fastestFee"], &f.fastest)) {
        return false;
    }
    *out = f;
    return true;
}

} // namespace explorer
