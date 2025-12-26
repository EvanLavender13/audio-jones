#include "modulation_engine.h"
#include <string>
#include <unordered_map>
#include <cstring>
#include <algorithm>

struct ParamMeta {
    float* ptr;
    float min;
    float max;
    float base;
};

static std::unordered_map<std::string, ParamMeta> sParams;
static std::unordered_map<std::string, ModRoute> sRoutes;
static std::unordered_map<std::string, float> sOffsets;

static float ApplyCurve(float x, ModCurve curve)
{
    switch (curve) {
        case MOD_CURVE_LINEAR:  return x;
        case MOD_CURVE_EXP:     return x * x;
        case MOD_CURVE_SQUARED: return x * x * x;
        default:                return x;
    }
}

void ModEngineInit(void)
{
    sParams.clear();
    sRoutes.clear();
    sOffsets.clear();
}

void ModEngineUninit(void)
{
    sParams.clear();
    sRoutes.clear();
    sOffsets.clear();
}

void ModEngineRegisterParam(const char* paramId, float* ptr, float min, float max)
{
    std::string id(paramId);

    // Check if already registered
    auto it = sParams.find(id);
    if (it != sParams.end()) {
        // Update pointer in case it changed, keep existing base
        it->second.ptr = ptr;
        it->second.min = min;
        it->second.max = max;
        return;
    }

    ParamMeta meta;
    meta.ptr = ptr;
    meta.min = min;
    meta.max = max;
    meta.base = *ptr;
    sParams[id] = meta;
    sOffsets[id] = 0.0f;
}

void ModEngineSetRoute(const char* paramId, const ModRoute* route)
{
    std::string id(paramId);
    sRoutes[id] = *route;
}

void ModEngineRemoveRoute(const char* paramId)
{
    std::string id(paramId);
    sRoutes.erase(id);
    sOffsets[id] = 0.0f;

    // Reset param to base value
    auto it = sParams.find(id);
    if (it != sParams.end() && it->second.ptr != NULL) {
        *it->second.ptr = it->second.base;
    }
}

bool ModEngineHasRoute(const char* paramId)
{
    std::string id(paramId);
    return sRoutes.find(id) != sRoutes.end();
}

bool ModEngineGetRoute(const char* paramId, ModRoute* outRoute)
{
    std::string id(paramId);
    auto it = sRoutes.find(id);
    if (it == sRoutes.end()) {
        return false;
    }
    *outRoute = it->second;
    return true;
}

void ModEngineUpdate(float dt, const ModSources* sources)
{
    (void)dt;  // Currently unused, may be needed for smoothing later

    for (auto& [id, route] : sRoutes) {
        auto paramIt = sParams.find(id);
        if (paramIt == sParams.end()) {
            continue;
        }

        ParamMeta& meta = paramIt->second;
        if (meta.ptr == NULL) {
            continue;
        }

        // Get source value (0-1)
        float sourceValue = 0.0f;
        if (route.source >= 0 && route.source < MOD_SOURCE_COUNT) {
            sourceValue = sources->values[route.source];
        }

        // Apply curve
        float curved = ApplyCurve(sourceValue, (ModCurve)route.curve);

        // Calculate offset: curved * amount * range
        float range = meta.max - meta.min;
        float offset = curved * route.amount * range;
        sOffsets[id] = offset;

        // Write modulated value (base + offset, clamped)
        float modulated = meta.base + offset;
        modulated = std::clamp(modulated, meta.min, meta.max);
        *meta.ptr = modulated;
    }
}

float ModEngineGetOffset(const char* paramId)
{
    std::string id(paramId);
    auto it = sOffsets.find(id);
    if (it == sOffsets.end()) {
        return 0.0f;
    }
    return it->second;
}

void ModEngineSetBase(const char* paramId, float base)
{
    std::string id(paramId);
    auto it = sParams.find(id);
    if (it != sParams.end()) {
        it->second.base = base;
    }
}

int ModEngineGetRouteCount(void)
{
    return (int)sRoutes.size();
}

bool ModEngineGetRouteByIndex(int index, ModRoute* outRoute)
{
    if (index < 0 || index >= (int)sRoutes.size()) {
        return false;
    }

    auto it = sRoutes.begin();
    std::advance(it, index);
    *outRoute = it->second;
    return true;
}

void ModEngineClearRoutes(void)
{
    // Reset all modulated params to base values before clearing
    for (auto& [id, route] : sRoutes) {
        auto paramIt = sParams.find(id);
        if (paramIt != sParams.end() && paramIt->second.ptr != NULL) {
            *paramIt->second.ptr = paramIt->second.base;
        }
        sOffsets[id] = 0.0f;
    }
    sRoutes.clear();
}

void ModEngineWriteBaseValues(void)
{
    for (auto& [id, meta] : sParams) {
        if (meta.ptr != NULL) {
            *meta.ptr = meta.base;
        }
    }
}
