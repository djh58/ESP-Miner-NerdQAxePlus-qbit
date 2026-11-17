#pragma once
#include <stddef.h>

// Checks the payout address in a stratum username against what qbit pools
// accept: `<address>[.<worker>]`, where the address is a bech32m P2MR address
// (witness version 2, 32-byte program). The qbit pool accepts any other
// username too but credits those shares to the pool's own fallback address, so
// the firmware warns about it; it never refuses to save or mine.
namespace qbit {

enum class Network { Unknown, Mainnet, Testnet, Regtest };

enum class PayoutStatus {
    Ok,              // a valid qbit P2MR address
    NotApplicable,   // not a qbit address and not a qbit pool: nothing to check
    Invalid,         // meant for qbit but not a valid P2MR address
    NetworkMismatch, // valid, but for a different network than the pool serves
};

struct PayoutCheck {
    PayoutStatus status;
    Network network;      // of the address, when it decoded
    const char *reason;   // short English reason, static storage; "" when Ok
};

// Length of the address part of a stratum username (before the first '.').
size_t stratum_user_base_len(const char *user);

// True when `base` starts with a qbit address prefix (qb1, tq1, qbrt1), in
// either case.
bool looks_like_qbit_address(const char *base, size_t len);

// Validates `len` bytes at `base` as a qbit payout address.
PayoutCheck check_address(const char *base, size_t len);

// Which network a pool endpoint serves, for known qbit pool endpoints;
// Unknown for anything else.
Network pool_network(const char *host, int port);

// The full check for one configured pool: validates the username's address
// when it looks like a qbit address or the pool is a known qbit pool, and
// flags an address for one network on a pool endpoint for another.
PayoutCheck check_stratum_user(const char *user, const char *pool_host, int pool_port);

const char *payout_status_name(PayoutStatus status);
const char *network_name(Network network);

} // namespace qbit
