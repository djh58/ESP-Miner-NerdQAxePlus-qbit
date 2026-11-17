#include <cctype>

#include "host_test.h"
#include "qbit_address.h"

using qbit::Network;
using qbit::PayoutStatus;

namespace {

// Real addresses: an operator payout address, the qbit pool's fallback
// address, and the testnet address from the pool's runbooks.
const char *kMainnet = "qb1zz2q6myu58k03q76zy8j0w4gkthcjt9kzmauq834339dmrl3d3ehqtjuhax";
const char *kMainnet2 = "qb1z6yl7zupwfkphu5tgk7yvk5p8c6hf6df6s9df5y3h9fx98fudnsesraf2rx";
const char *kTestnet = "tq1zx9588hlvcnqc928ytjkk7txkds3rve9hjwyuqlfm0yjqthzkfucqhrytw3";
// Generated with the BIP-350 reference encoder.
const char *kRegtest = "qbrt1zqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqp5yfy4";
const char *kBech32NotM = "qb1zqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqp7qmfa";
const char *kWitnessV0 = "qb1qqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqkgf36g";
const char *kWitnessV1 = "qb1pqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqulfcz5";
const char *kProgram20 = "qb1zqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5gju5xa";
const char *kBitcoinP2tr = "bc1pqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqwk0jyn";

qbit::PayoutCheck check(const std::string &s) { return qbit::check_address(s.c_str(), s.size()); }

std::string upper(std::string s)
{
    for (char &c : s) c = (char) toupper((unsigned char) c);
    return s;
}

} // namespace

void test_qbit_address()
{
    CHECK(check(kMainnet).status == PayoutStatus::Ok);
    CHECK(check(kMainnet).network == Network::Mainnet);
    CHECK(check(kMainnet2).status == PayoutStatus::Ok);
    CHECK(check(kTestnet).status == PayoutStatus::Ok);
    CHECK(check(kTestnet).network == Network::Testnet);
    CHECK(check(kRegtest).network == Network::Regtest);
    CHECK(check(upper(kMainnet)).status == PayoutStatus::Ok);

    // The username that sent a miner's shares to the pool fallback.
    CHECK(check("87b78bb1e8").status == PayoutStatus::Invalid);
    CHECK_STR(check("87b78bb1e8").reason, "not a qbit address (mainnet addresses start qb1z)");

    std::string mixed = kMainnet;
    mixed[10] = (char) toupper((unsigned char) mixed[10]);
    CHECK_STR(check(mixed).reason, "mixes upper and lower case");

    std::string typo = kMainnet;
    typo[20] = typo[20] == 'q' ? 'p' : 'q';
    CHECK_STR(check(typo).reason, "checksum does not match (typo or truncated)");
    std::string truncated = std::string(kMainnet).substr(0, strlen(kMainnet) - 1);
    CHECK_STR(check(truncated).reason, "checksum does not match (typo or truncated)");

    CHECK_STR(check(kBech32NotM).reason, "old bech32 checksum; payout addresses use bech32m");
    CHECK_STR(check(kWitnessV0).reason, "old bech32 checksum; payout addresses use bech32m");
    CHECK_STR(check(kWitnessV1).reason, "not a P2MR address (needs witness version 2, starts qb1z)");
    CHECK_STR(check(kProgram20).reason, "P2MR program must be 32 bytes");
    CHECK_STR(check(kBitcoinP2tr).reason, "not a qbit address (mainnet addresses start qb1z)");
    CHECK_STR(check("").reason, "empty address");
    CHECK_STR(check(std::string(91, 'q')).reason, "too long for an address");
    CHECK_STR(check("qb1zq\x01q").reason, "contains a character addresses never use");
    CHECK_STR(check("qb1zqb").reason, "too short");

    CHECK(qbit::stratum_user_base_len("qb1abc.home") == 6);
    CHECK(qbit::stratum_user_base_len("qb1abc") == 6);
    CHECK(qbit::stratum_user_base_len("") == 0);
    CHECK(qbit::stratum_user_base_len(nullptr) == 0);

    std::string user = std::string(kMainnet) + ".home";
    CHECK(qbit::check_stratum_user(user.c_str(), "pool.qbit.org", 4335).status == PayoutStatus::Ok);
    CHECK(qbit::check_stratum_user(kMainnet, "mine.prismpool.io", 4334).status == PayoutStatus::Ok);
    CHECK(qbit::check_stratum_user(user.c_str(), "POOL.QBIT.ORG.", 4335).status == PayoutStatus::Ok);
    CHECK(qbit::check_stratum_user("87b78bb1e8.home", "pool.qbit.org", 4335).status == PayoutStatus::Invalid);
    CHECK(qbit::check_stratum_user("", "pool.qbit.org", 4335).status == PayoutStatus::Invalid);
    CHECK(qbit::check_stratum_user(nullptr, "pool.qbit.org", 4335).status == PayoutStatus::Invalid);

    std::string testnet_user = std::string(kTestnet) + ".rig";
    auto mismatch = qbit::check_stratum_user(testnet_user.c_str(), "pool.qbit.org", 4335);
    CHECK(mismatch.status == PayoutStatus::NetworkMismatch);
    CHECK_STR(mismatch.reason, "testnet address on a mainnet pool");
    CHECK(qbit::check_stratum_user(user.c_str(), "mine.prismpool.io", 3344).status == PayoutStatus::NetworkMismatch);
    CHECK(qbit::check_stratum_user(testnet_user.c_str(), "mine.prismpool.io", 3344).status == PayoutStatus::Ok);
    // An unrecognized port on a qbit host checks the address but not the network.
    CHECK(qbit::check_stratum_user(testnet_user.c_str(), "mine.prismpool.io", 9999).status == PayoutStatus::Ok);

    // Other pools and usernames are left alone, unless the username is meant
    // to be a qbit address.
    CHECK(qbit::check_stratum_user("bc1qexample.worker", "public-pool.io", 21496).status == PayoutStatus::NotApplicable);
    CHECK(qbit::check_stratum_user("myworker", "solo.ckpool.org", 3333).status == PayoutStatus::NotApplicable);
    CHECK(qbit::check_stratum_user(typo.c_str(), "some-pool.example", 3333).status == PayoutStatus::Invalid);
    CHECK(qbit::check_stratum_user(nullptr, nullptr, 0).status == PayoutStatus::NotApplicable);
}
