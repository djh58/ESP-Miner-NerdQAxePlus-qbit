#include "qbit_chain.h"

#include <cmath>
#include <cstdio>

namespace qbit {

int64_t block_subsidy(uint32_t height)
{
    int64_t subsidy = kSubsidyInitial;
    const uint32_t steps = height / kSubsidyStepInterval;
    for (uint32_t step = 0; step < steps && subsidy > 0; ++step) {
        subsidy = (subsidy / kStepdownDenominator) * kStepdownNumerator +
                  ((subsidy % kStepdownDenominator) * kStepdownNumerator) / kStepdownDenominator;
    }
    return subsidy;
}

uint32_t blocks_to_next_stepdown(uint32_t height)
{
    return kSubsidyStepInterval - (height % kSubsidyStepInterval);
}

uint32_t stepdown_progress_percent(uint32_t height)
{
    return (uint32_t) ((uint64_t) (height % kSubsidyStepInterval) * 100 / kSubsidyStepInterval);
}

void format_hashrate(double hps, char *value, size_t value_len, const char **unit)
{
    static const char *const units[] = {"H/s", "KH/s", "MH/s", "GH/s", "TH/s", "PH/s", "EH/s"};
    if (!std::isfinite(hps) || hps < 0) {
        snprintf(value, value_len, "--");
        *unit = "";
        return;
    }
    size_t i = 0;
    while (hps >= 1000.0 && i + 1 < sizeof(units) / sizeof(units[0])) {
        hps /= 1000.0;
        i++;
    }
    snprintf(value, value_len, hps >= 100.0 ? "%.0f" : "%.1f", hps);
    *unit = units[i];
}

void format_difficulty(double d, char *out, size_t out_len)
{
    static const char suffixes[] = {'\0', 'K', 'M', 'G', 'T', 'P', 'E'};
    if (!std::isfinite(d) || d < 0) {
        snprintf(out, out_len, "--");
        return;
    }
    size_t i = 0;
    while (d >= 1000.0 && i + 1 < sizeof(suffixes)) {
        d /= 1000.0;
        i++;
    }
    if (i == 0) {
        snprintf(out, out_len, "%.0f", d);
    } else if (d >= 100.0) {
        snprintf(out, out_len, "%.0f%c", d, suffixes[i]);
    } else if (d >= 10.0) {
        snprintf(out, out_len, "%.1f%c", d, suffixes[i]);
    } else {
        snprintf(out, out_len, "%.2f%c", d, suffixes[i]);
    }
}

void format_qbt(int64_t base_units, char *out, size_t out_len)
{
    const char *sign = base_units < 0 ? "-" : "";
    uint64_t v = base_units < 0 ? (uint64_t) (-(base_units + 1)) + 1 : (uint64_t) base_units;
    snprintf(out, out_len, "%s%llu.%02llu", sign, (unsigned long long) (v / kCoin),
             (unsigned long long) (v % kCoin / (kCoin / 100)));
}

} // namespace qbit
