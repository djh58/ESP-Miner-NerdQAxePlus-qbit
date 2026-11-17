#include <cmath>

#include "host_test.h"
#include "qbit_chain.h"

namespace {

std::string hashrate(double hps)
{
    char value[16];
    const char *unit = nullptr;
    qbit::format_hashrate(hps, value, sizeof(value), &unit);
    return std::string(value) + " " + unit;
}

std::string difficulty(double d)
{
    char out[16];
    qbit::format_difficulty(d, out, sizeof(out));
    return out;
}

std::string qbt(int64_t units)
{
    char out[24];
    qbit::format_qbt(units, out, sizeof(out));
    return out;
}

} // namespace

void test_qbit_chain()
{
    // Expected values follow GetBlockSubsidy's integer formula step by step.
    CHECK(qbit::block_subsidy(0) == 21000000000LL);
    CHECK(qbit::block_subsidy(43199) == 21000000000LL);
    CHECK(qbit::block_subsidy(43200) == 20092800000LL);
    CHECK(qbit::block_subsidy(86400) == 19224791040LL);
    CHECK(qbit::block_subsidy(129600) == 18394280067LL);
    CHECK(qbit::block_subsidy(178805) == 17599647168LL);
    CHECK(qbit::block_subsidy(216000) == 16839342410LL);

    CHECK(qbit::blocks_to_next_stepdown(0) == 43200);
    CHECK(qbit::blocks_to_next_stepdown(43199) == 1);
    CHECK(qbit::blocks_to_next_stepdown(43200) == 43200);
    CHECK(qbit::blocks_to_next_stepdown(178805) == 37195);
    CHECK(qbit::stepdown_progress_percent(0) == 0);
    CHECK(qbit::stepdown_progress_percent(43199) == 99);
    CHECK(qbit::stepdown_progress_percent(178805) == 13); // 6,005 of 43,200

    // Live explorer values from 2026-11-17.
    CHECK_STR(hashrate(86415836911458600.0), "86.4 PH/s");
    CHECK_STR(hashrate(357658047446749540000.0), "358 EH/s");
    CHECK_STR(hashrate(4822e9), "4.8 TH/s");
    CHECK_STR(hashrate(0), "0.0 H/s");
    CHECK_STR(hashrate(NAN), "-- ");
    CHECK_STR(hashrate(-1), "-- ");

    CHECK_STR(difficulty(1348860090.875964), "1.35G");
    CHECK_STR(difficulty(20879651960271.07), "20.9T");
    CHECK_STR(difficulty(134823837323272.7), "135T");
    CHECK_STR(difficulty(512), "512");
    CHECK_STR(difficulty(INFINITY), "--");

    CHECK_STR(qbt(17599647168LL), "175.99");
    CHECK_STR(qbt(21000000000LL), "210.00");
    CHECK_STR(qbt(0), "0.00");
    CHECK_STR(qbt(-150000000LL), "-1.50");
}
