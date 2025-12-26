#include "modulation_config.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <algorithm>

using json = nlohmann::json;

static void to_json(json& j, const ModRoute& r)
{
    j["paramId"] = std::string(r.paramId);
    j["source"] = r.source;
    j["amount"] = r.amount;
    j["curve"] = r.curve;
}

static void from_json(const json& j, ModRoute& r)
{
    memset(&r, 0, sizeof(ModRoute));

    if (j.contains("paramId")) {
        const std::string id = j["paramId"].get<std::string>();
        strncpy(r.paramId, id.c_str(), sizeof(r.paramId) - 1);
        r.paramId[sizeof(r.paramId) - 1] = '\0';
    }
    if (j.contains("source")) {
        r.source = j["source"].get<int>();
    }
    if (j.contains("amount")) {
        r.amount = j["amount"].get<float>();
    }
    if (j.contains("curve")) {
        r.curve = j["curve"].get<int>();
    }
}

void to_json(json& j, const ModulationConfig& c)
{
    j["routes"] = json::array();
    for (int i = 0; i < c.count; i++) {
        json routeJson;
        ::to_json(routeJson, c.routes[i]);
        j["routes"].push_back(routeJson);
    }
}

void from_json(const json& j, ModulationConfig& c)
{
    c.count = 0;

    if (!j.contains("routes")) {
        return;
    }

    const auto& arr = j["routes"];
    const int count = std::min((int)arr.size(), MAX_MOD_ROUTES);
    for (int i = 0; i < count; i++) {
        ::from_json(arr[i], c.routes[i]);
    }
    c.count = count;
}

void ModulationConfigFromEngine(ModulationConfig* config)
{
    config->count = 0;

    const int routeCount = ModEngineGetRouteCount();
    const int maxRoutes = std::min(routeCount, MAX_MOD_ROUTES);

    for (int i = 0; i < maxRoutes; i++) {
        if (ModEngineGetRouteByIndex(i, &config->routes[i])) {
            config->count++;
        }
    }
}

void ModulationConfigToEngine(const ModulationConfig* config)
{
    ModEngineClearRoutes();
    ModEngineSyncBases();

    for (int i = 0; i < config->count; i++) {
        const ModRoute* route = &config->routes[i];
        if (route->paramId[0] != '\0') {
            ModEngineSetRoute(route->paramId, route);
        }
    }
}
