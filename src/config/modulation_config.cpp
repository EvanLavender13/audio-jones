#include "modulation_config.h"
#include "config/effect_descriptor.h"
#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static void to_json(json &j, const ModRoute &r) {
  j["paramId"] = std::string(r.paramId);
  j["source"] = r.source;
  j["amount"] = r.amount;
  j["curve"] = r.curve;
}

static void from_json(const json &j, ModRoute &r) {
  r = ModRoute{};

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

void to_json(json &j, const ModulationConfig &c) {
  j["routes"] = json::array();
  for (int i = 0; i < c.count; i++) {
    json routeJson;
    ::to_json(routeJson, c.routes[i]);
    j["routes"].push_back(routeJson);
  }
}

void from_json(const json &j, ModulationConfig &c) {
  c.count = 0;

  if (!j.contains("routes")) {
    return;
  }

  const auto &arr = j["routes"];
  const int count = std::min((int)arr.size(), MAX_MOD_ROUTES);
  for (int i = 0; i < count; i++) {
    ::from_json(arr[i], c.routes[i]);
  }
  c.count = count;
}

void ModulationConfigFromEngine(ModulationConfig *config) {
  config->count = 0;

  const int routeCount = ModEngineGetRouteCount();
  const int maxRoutes = std::min(routeCount, MAX_MOD_ROUTES);

  for (int i = 0; i < maxRoutes; i++) {
    if (ModEngineGetRouteByIndex(i, &config->routes[i])) {
      config->count++;
    }
  }
}

void ModulationConfigToEngine(const ModulationConfig *config) {
  ModEngineSyncBases(); // Capture new preset values before clearing routes
  ModEngineClearRoutes();

  for (int i = 0; i < config->count; i++) {
    const ModRoute *route = &config->routes[i];
    if (route->paramId[0] != '\0') {
      ModEngineSetRoute(route->paramId, route);
    }
  }
}

void ModulationConfigStripDisabledRoutes(ModulationConfig *config,
                                         const EffectConfig *effects) {
  int writeIdx = 0;

  for (int i = 0; i < config->count; i++) {
    const char *paramId = config->routes[i].paramId;

    // Find the prefix: everything up to and including the first '.'
    const char *dot = strchr(paramId, '.');
    bool remove = false;

    if (dot != nullptr) {
      const size_t prefixLen = (size_t)(dot - paramId) + 1; // include the '.'

      for (int d = 0; d < TRANSFORM_EFFECT_COUNT; d++) {
        const EffectDescriptor &desc = EFFECT_DESCRIPTORS[d];
        if (desc.paramPrefix == nullptr) {
          continue;
        }
        if (strncmp(paramId, desc.paramPrefix, prefixLen) == 0 &&
            desc.paramPrefix[prefixLen] == '\0') {
          // Prefix matches — check if effect is disabled
          const bool enabled = *reinterpret_cast<const bool *>(
              reinterpret_cast<const char *>(effects) + desc.enabledOffset);
          if (!enabled) {
            remove = true;
          }
          break;
        }
      }
    }

    if (!remove) {
      if (writeIdx != i) {
        config->routes[writeIdx] = config->routes[i];
      }
      writeIdx++;
    }
  }

  config->count = writeIdx;
}
