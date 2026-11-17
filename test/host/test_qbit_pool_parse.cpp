#include <cmath>
#include <fstream>
#include <sstream>

#include "host_test.h"
#include "qbit_pool_parse.h"

namespace {

std::string fixture(const char *name)
{
    std::ifstream in(std::string(FIXTURE_DIR) + "/" + name);
    std::stringstream s;
    s << in.rdbuf();
    return s.str();
}

bool near(double a, double b) { return std::fabs(a - b) <= std::fabs(b) * 1e-9; }

} // namespace

void test_qbit_pool_parse()
{
    // Live responses from www.prismpool.io/public/v1 on 2026-11-17.
    std::string body = fixture("qbitpool-pool-summary.json");
    qbitpool::PoolSummary p = {};
    CHECK(qbitpool::parse_pool_summary(body.data(), body.size(), &p));
    CHECK(near(p.hashrateThs, 50241.96659493973419921184647));
    CHECK(p.participants3h == 30);
    CHECK(p.expectedBlockSeconds == 125);
    CHECK(p.latestBlockHeight == 178869);
    CHECK(p.latestBlockAgeSeconds == 139);
    CHECK(!qbitpool::parse_pool_summary("{\"pool\":{}}", 11, &p));
    CHECK(!qbitpool::parse_pool_summary(body.data(), body.size() / 2, &p));

    body = fixture("qbitpool-miner.json");
    qbitpool::MinerStats m = {};
    CHECK(qbitpool::parse_miner(body.data(), body.size(), "home", &m));
    CHECK(near(m.windowPercent, 1.5550572036727069));
    CHECK(m.nextBlockRewardBits == 268210889);
    CHECK(m.pendingBits == 162111102222);
    CHECK(m.unpaidBits == 685);
    CHECK(m.lifetimeBits == 942693892650);
    CHECK(near(m.hashrateThs, 751.0286433429279020264100117));
    CHECK(m.workerFound);
    CHECK(near(m.workerHashrateThs, 4.087535948376592758052485997));
    CHECK(m.hasLastPayout && m.lastPayoutHeight == 178869 && m.lastPayoutBits == 271804096);

    CHECK(qbitpool::parse_miner(body.data(), body.size(), "missing", &m) && !m.workerFound);
    CHECK(qbitpool::parse_miner(body.data(), body.size(), "", &m) && !m.workerFound);
    CHECK(qbitpool::parse_miner(body.data(), body.size(), nullptr, &m) && !m.workerFound);
    CHECK(!qbitpool::parse_miner("{\"reward_window_percent\":\"abc\"}", 31, "home", &m));
    CHECK(!qbitpool::parse_miner("{\"reward_window_percent\":\"-1\"}", 30, "home", &m));
}
