#include "sim_scenario.h"

#include <fstream>
#include <sstream>

#include "ArduinoJson.h"

namespace {

template <typename T> void read(JsonVariantConst obj, const char *key, T &into)
{
    JsonVariantConst v = obj[key];
    if (!v.isNull()) into = v.as<T>();
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
    return true;
}
