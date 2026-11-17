#include <cmath>
#include <fstream>
#include <sstream>

#include "explorer_parse.h"
#include "host_test.h"

namespace {

// Live responses saved on 2026-11-17.
std::string fixture(const char *name)
{
    std::ifstream in(std::string(FIXTURE_DIR) + "/" + name);
    std::stringstream s;
    s << in.rdbuf();
    if (s.str().empty()) {
        g_failures++;
        fprintf(stderr, "missing fixture %s\n", name);
    }
    return s.str();
}

bool near(double a, double b) { return std::fabs(a - b) <= std::fabs(b) * 1e-9; }

} // namespace

void test_explorer_parse()
{
    uint32_t u = 0;
    std::string body = fixture("mempool.space-v1_prices.json");
    CHECK(explorer::parse_price_usd(body.data(), body.size(), &u) && u == 64442);
    body = fixture("mempool.qbit.org-v1_prices.json"); // "USD": -1, no price feed
    CHECK(!explorer::parse_price_usd(body.data(), body.size(), &u));
    CHECK(!explorer::parse_price_usd("{\"USD\":0}", 9, &u));
    CHECK(!explorer::parse_price_usd("{\"EUR\":5}", 9, &u));

    body = fixture("mempool.space-blocks_tip_height.json");
    CHECK(explorer::parse_block_height(body.data(), body.size(), &u) && u == 976212);
    body = fixture("mempool.qbit.org-blocks_tip_height.json");
    CHECK(explorer::parse_block_height(body.data(), body.size(), &u) && u == 178805);
    CHECK(!explorer::parse_block_height("-1", 2, &u));
    CHECK(!explorer::parse_block_height("0", 1, &u));
    CHECK(!explorer::parse_block_height("12.5", 4, &u));
    CHECK(!explorer::parse_block_height("<html>", 6, &u));

    explorer::NetworkHashrate hr = {};
    body = fixture("mempool.space-v1_mining_hashrate_3d.json");
    CHECK(explorer::parse_network_hashrate(body.data(), body.size(), &hr));
    CHECK(!hr.qbit_lanes);
    CHECK(near(hr.hashrate, 964605107254170200000.0));
    CHECK(near(hr.difficulty, 134823837323272.7));

    body = fixture("mempool.qbit.org-v1_mining_hashrate_3d.json");
    CHECK(body.size() > 1024); // larger than the upstream firmware's 1 KB buffer
    CHECK(explorer::parse_network_hashrate(body.data(), body.size(), &hr));
    CHECK(hr.qbit_lanes);
    CHECK(near(hr.hashrate, 86415836911458600.0));
    CHECK(near(hr.difficulty, 1348860090.875964));
    // What the upstream buffer kept: the first 1023 bytes must fail, not parse.
    CHECK(!explorer::parse_network_hashrate(body.data(), 1023, &hr));
    CHECK(!explorer::parse_network_hashrate("{}", 2, &hr));

    explorer::Fees fees = {};
    body = fixture("mempool.qbit.org-v1_fees_recommended.json");
    CHECK(explorer::parse_fees(body.data(), body.size(), &fees) && fees.hour == 1 && fees.fastest == 1);
    CHECK(!explorer::parse_fees("{\"hourFee\":1}", 13, &fees));
    CHECK(!explorer::parse_fees("{\"hourFee\":-1,\"halfHourFee\":1,\"fastestFee\":1}", 45, &fees));
}
