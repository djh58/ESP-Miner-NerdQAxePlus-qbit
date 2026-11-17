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

    read(doc["price"].as<JsonVariantConst>(), "usd", s.priceUsd);

    JsonVariantConst n = doc["network"];
    read(n, "blockHeight", s.blockHeight);
    read(n, "hashrateHps", s.netHashrateHps);
    read(n, "difficulty", s.netDifficulty);
    read(n, "feeLow", s.feeLow);
    read(n, "feeMid", s.feeMid);
    read(n, "feeHigh", s.feeHigh);
    return true;
}
