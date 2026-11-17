#include "qbit_address.h"

#include <cctype>
#include <cstdint>
#include <cstring>

namespace qbit {
namespace {

constexpr uint32_t kBech32Const = 1;
constexpr uint32_t kBech32mConst = 0x2bc830a3;
constexpr size_t kMaxLength = 90;
const char kCharset[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

uint32_t polymod_step(uint32_t chk, uint8_t value)
{
    static const uint32_t gen[5] = {0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3};
    uint8_t top = chk >> 25;
    chk = ((chk & 0x1ffffff) << 5) ^ value;
    for (int i = 0; i < 5; i++) {
        if ((top >> i) & 1) chk ^= gen[i];
    }
    return chk;
}

int charset_index(char c)
{
    const char *p = strchr(kCharset, c);
    return (p && c) ? (int) (p - kCharset) : -1;
}

bool equals_lower(const char *s, size_t len, const char *lower)
{
    if (strlen(lower) != len) return false;
    for (size_t i = 0; i < len; i++) {
        if (tolower((unsigned char) s[i]) != lower[i]) return false;
    }
    return true;
}

bool host_equals(const char *host, const char *name)
{
    size_t n = strlen(name);
    if (!host) return false;
    size_t h = strlen(host);
    // Accept a trailing dot (fully qualified form).
    if (h == n + 1 && host[n] == '.') h = n;
    if (h != n) return false;
    for (size_t i = 0; i < n; i++) {
        if (tolower((unsigned char) host[i]) != name[i]) return false;
    }
    return true;
}

PayoutCheck invalid(const char *reason) { return {PayoutStatus::Invalid, Network::Unknown, reason}; }

} // namespace

size_t stratum_user_base_len(const char *user)
{
    if (!user) return 0;
    const char *dot = strchr(user, '.');
    return dot ? (size_t) (dot - user) : strlen(user);
}

bool looks_like_qbit_address(const char *base, size_t len)
{
    static const char *const prefixes[] = {"qb1", "tq1", "qbrt1"};
    for (const char *p : prefixes) {
        size_t n = strlen(p);
        if (len >= n && equals_lower(base, n, p)) return true;
    }
    return false;
}

PayoutCheck check_address(const char *s, size_t len)
{
    if (!s || len == 0) return invalid("empty address");
    if (len > kMaxLength) return invalid("too long for an address");

    bool has_lower = false, has_upper = false;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char) s[i];
        if (c < 33 || c > 126) return invalid("contains a character addresses never use");
        if (islower(c)) has_lower = true;
        if (isupper(c)) has_upper = true;
    }
    if (has_lower && has_upper) return invalid("mixes upper and lower case");

    size_t sep = len;
    for (size_t i = len; i-- > 0;) {
        if (s[i] == '1') {
            sep = i;
            break;
        }
    }
    const char *not_qbit = "not a qbit address (mainnet addresses start qb1z)";
    if (sep == len || sep == 0) return invalid(not_qbit);

    Network network;
    if (equals_lower(s, sep, "qb")) network = Network::Mainnet;
    else if (equals_lower(s, sep, "tq")) network = Network::Testnet;
    else if (equals_lower(s, sep, "qbrt")) network = Network::Regtest;
    else return invalid(not_qbit);
    if (len - sep - 1 < 6) return invalid("too short");

    uint32_t chk = 1;
    for (size_t i = 0; i < sep; i++) chk = polymod_step(chk, tolower((unsigned char) s[i]) >> 5);
    chk = polymod_step(chk, 0);
    for (size_t i = 0; i < sep; i++) chk = polymod_step(chk, tolower((unsigned char) s[i]) & 31);

    uint8_t data[kMaxLength];
    size_t data_len = 0;
    for (size_t i = sep + 1; i < len; i++) {
        int v = charset_index((char) tolower((unsigned char) s[i]));
        if (v < 0) return invalid("contains a character addresses never use");
        data[data_len++] = (uint8_t) v;
        chk = polymod_step(chk, (uint8_t) v);
    }

    if (chk == kBech32Const) return invalid("old bech32 checksum; payout addresses use bech32m");
    if (chk != kBech32mConst) return invalid("checksum does not match (typo or truncated)");

    // data = witness version, program (5-bit groups), 6 checksum symbols
    if (data_len < 7) return invalid("too short");
    uint8_t version = data[0];
    if (version != 2) return invalid("not a P2MR address (needs witness version 2, starts qb1z)");

    uint32_t acc = 0;
    int bits = 0;
    size_t program_len = 0;
    for (size_t i = 1; i < data_len - 6; i++) {
        acc = (acc << 5) | data[i];
        bits += 5;
        if (bits >= 8) {
            bits -= 8;
            program_len++;
        }
    }
    if (bits >= 5 || (acc & ((1u << bits) - 1)) != 0) return invalid("invalid padding");
    if (program_len != 32) return invalid("P2MR program must be 32 bytes");

    return {PayoutStatus::Ok, network, ""};
}

Network pool_network(const char *host, int port)
{
    if (host_equals(host, "pool.qbit.org")) return Network::Mainnet;
    if (host_equals(host, "mine.prismpool.io")) {
        if (port == 4334 || port == 4335) return Network::Mainnet;
        if (port == 3344) return Network::Testnet;
        return Network::Unknown;
    }
    if (host_equals(host, "testnet-stratum.swaplabs.xyz")) return Network::Testnet;
    return Network::Unknown;
}

PayoutCheck check_stratum_user(const char *user, const char *pool_host, int pool_port)
{
    size_t base_len = stratum_user_base_len(user);
    Network pool = pool_network(pool_host, pool_port);
    bool qbit_pool = pool_host && (host_equals(pool_host, "pool.qbit.org") || host_equals(pool_host, "mine.prismpool.io") ||
                                   host_equals(pool_host, "testnet-stratum.swaplabs.xyz"));

    if (!qbit_pool && !looks_like_qbit_address(user ? user : "", base_len)) {
        return {PayoutStatus::NotApplicable, Network::Unknown, ""};
    }

    PayoutCheck check = check_address(user, base_len);
    if (check.status != PayoutStatus::Ok) return check;

    if (pool != Network::Unknown && check.network != pool) {
        return {PayoutStatus::NetworkMismatch, check.network,
                pool == Network::Mainnet ? "testnet address on a mainnet pool" : "mainnet address on a testnet pool"};
    }
    return check;
}

const char *payout_status_name(PayoutStatus status)
{
    switch (status) {
    case PayoutStatus::Ok: return "ok";
    case PayoutStatus::NotApplicable: return "not_applicable";
    case PayoutStatus::Invalid: return "invalid";
    case PayoutStatus::NetworkMismatch: return "network_mismatch";
    }
    return "unknown";
}

const char *network_name(Network network)
{
    switch (network) {
    case Network::Mainnet: return "mainnet";
    case Network::Testnet: return "testnet";
    case Network::Regtest: return "regtest";
    case Network::Unknown: break;
    }
    return "unknown";
}

} // namespace qbit
