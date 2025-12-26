#ifndef MODULATION_CONFIG_H
#define MODULATION_CONFIG_H

#include "automation/modulation_engine.h"
#include <nlohmann/json_fwd.hpp>

#define MAX_MOD_ROUTES 64

struct ModulationConfig {
    ModRoute routes[MAX_MOD_ROUTES];
    int count = 0;
};

// JSON serialization (required for preset.cpp - see preset.cpp known deviation)
void to_json(nlohmann::json& j, const ModulationConfig& c);
void from_json(const nlohmann::json& j, ModulationConfig& c);

void ModulationConfigFromEngine(ModulationConfig* config);
void ModulationConfigToEngine(const ModulationConfig* config);

#endif // MODULATION_CONFIG_H
