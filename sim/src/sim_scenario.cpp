#include "sim_scenario.h"

#include <fstream>
#include <sstream>

#include "ArduinoJson.h"
#include "qbit_pool_parse.h"

namespace {

template <typename T> void read(JsonVariantConst obj, const char *key, T &into)
{
    JsonVariantConst v = obj[key];
    if (!v.isNull()) into = v.as<T>();
}


std::string read_file(const std::string &path)
{
    std::ifstream in(path);
    std::stringstream buf;
    buf << in.rdbuf();
    return buf.str();
}

std::string sibling(const std::string &scenario_path, const std::string &relative)
{
    size_t slash = scenario_path.find_last_of('/');
    return slash == std::string::npos ? relative : scenario_path.substr(0, slash + 1) + relative;
}
} // namespace

bool load_scenario(const std::string &path, SimScenario &s, std::string &error)
{
    std::ifstream in(path);
    if (!in) {
        error = "cannot read " + path;
        return false;
    }
    std::stringstream buf;
    buf << in.rdbuf();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, buf.str());
    if (err) {
        error = path + ": " + err.c_str();
        return false;
    }

    s = SimScenario();
    read(doc.as<JsonVariantConst>(), "name", s.name);

    JsonVariantConst m = doc["mining"];
    read(m, "hashrateGhs", s.hashrateGhs);
    read(m, "powerW", s.powerW);
    read(m, "chipTempC", s.chipTempC);
    read(m, "fanRpm", s.fanRpm);
    read(m, "currentMa", s.currentMa);
    read(m, "vinMv", s.vinMv);
    read(m, "vcoreMv", s.vcoreMv);
    read(m, "uptimeSeconds", s.uptimeSeconds);
    read(m, "sharesAccepted", s.sharesAccepted);
    read(m, "sharesRejected", s.sharesRejected);
    read(m, "bestDiff", s.bestDiff);
    read(m, "ip", s.ip);

    JsonVariantConst st = doc["settings"];
    read(st, "poolHost", s.poolHost);
    read(st, "poolPort", s.poolPort);
    read(st, "frequencyMhz", s.frequencyMhz);
    read(st, "asicVoltageMv", s.asicVoltageMv);
    read(st, "fanMode", s.fanMode);
    read(st, "fanSpeedPercent", s.fanSpeedPercent);
    read(st, "stratumUser", s.stratumUser);
    read(st, "fallbackHost", s.fallbackHost);
    read(st, "fallbackPort", s.fallbackPort);
    read(st, "fallbackUser", s.fallbackUser);

    JsonVariantConst n = doc["network"];
    NetworkStats &ns = s.network;
    read(n, "qbit", ns.qbit);
    ns.priceKnown = !n["priceUsd"].isNull();
    read(n, "priceUsd", ns.priceUsd);
    ns.heightKnown = !n["blockHeight"].isNull();
    read(n, "blockHeight", ns.blockHeight);
    ns.hashrateKnown = !n["hashrateHps"].isNull() && !n["difficulty"].isNull();
    read(n, "hashrateHps", ns.hashrate);
    read(n, "difficulty", ns.difficulty);
    ns.feesKnown = !n["fees"].isNull();
    read(n["fees"], "hour", ns.feeHour);
    read(n["fees"], "halfHour", ns.feeHalfHour);
    read(n["fees"], "fastest", ns.feeFastest);

    JsonVariantConst qp = doc["qbitPool"];
    if (!qp.isNull()) {
        s.hasPool = true;
        QbitPoolView &v = s.pool;
        snprintf(v.address, sizeof(v.address), "%s", qp["address"] | "");
        snprintf(v.worker, sizeof(v.worker), "%s", qp["worker"] | "");
        v.sinceFetchSeconds = qp["sinceFetchSeconds"] | 0;
        if (!qp["poolSummary"].isNull()) {
            std::string body = read_file(sibling(path, qp["poolSummary"].as<std::string>()));
            v.poolKnown = qbitpool::parse_pool_summary(body.data(), body.size(), &v.pool);
            if (!v.poolKnown) fprintf(stderr, "qbitPool.poolSummary did not parse\n");
        }
        if (!qp["miner"].isNull()) {
            std::string body = read_file(sibling(path, qp["miner"].as<std::string>()));
            v.minerKnown = qbitpool::parse_miner(body.data(), body.size(), v.worker, &v.miner);
            if (!v.minerKnown) fprintf(stderr, "qbitPool.miner did not parse\n");
        }
    }

    JsonVariantConst t = doc["toast"];
    if (!t.isNull()) {
        s.hasToast = true;
        s.toast.height = t["height"] | 0u;
        s.toast.paid = !t["paidBits"].isNull();
        s.toast.paidBits = t["paidBits"] | (int64_t) 0;
        s.toast.sharePercent = t["sharePercent"] | 0.0;
    }
    return true;
}
