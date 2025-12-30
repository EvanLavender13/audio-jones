#include "param_registry.h"
#include "modulation_engine.h"
#include "config/effect_config.h"
#include <string.h>

typedef struct ParamEntry {
    const char* id;
    ParamDef def;
} ParamEntry;

static const ParamEntry PARAM_TABLE[] = {
    {"physarum.sensorDistance", {1.0f, 100.0f}},
    {"physarum.sensorAngle",    {0.0f, 6.28f}},
    {"physarum.turningAngle",   {0.0f, 6.28f}},
    {"physarum.stepSize",       {0.1f, 100.0f}},
    {"effects.blurScale",       {0.0f, 10.0f}},
    {"effects.chromaticOffset", {0.0f, 50.0f}},
    {"flowField.zoomBase",      {0.98f, 1.02f}},
    {"flowField.zoomRadial",    {-0.02f, 0.02f}},
    {"flowField.rotBase",       {-0.01f, 0.01f}},
    {"flowField.rotRadial",     {-0.01f, 0.01f}},
    {"flowField.dxBase",        {-0.02f, 0.02f}},
    {"flowField.dxRadial",      {-0.02f, 0.02f}},
    {"flowField.dyBase",        {-0.02f, 0.02f}},
    {"flowField.dyRadial",      {-0.02f, 0.02f}},
};

static const int PARAM_COUNT = sizeof(PARAM_TABLE) / sizeof(PARAM_TABLE[0]);

void ParamRegistryInit(EffectConfig* effects)
{
    float* targets[] = {
        &effects->physarum.sensorDistance,
        &effects->physarum.sensorAngle,
        &effects->physarum.turningAngle,
        &effects->physarum.stepSize,
        &effects->blurScale,
        &effects->chromaticOffset,
        &effects->flowField.zoomBase,
        &effects->flowField.zoomRadial,
        &effects->flowField.rotBase,
        &effects->flowField.rotRadial,
        &effects->flowField.dxBase,
        &effects->flowField.dxRadial,
        &effects->flowField.dyBase,
        &effects->flowField.dyRadial,
    };

    for (int i = 0; i < PARAM_COUNT; i++) {
        ModEngineRegisterParam(PARAM_TABLE[i].id, targets[i],
                               PARAM_TABLE[i].def.min, PARAM_TABLE[i].def.max);
    }
}

const ParamDef* ParamRegistryGet(const char* paramId)
{
    for (int i = 0; i < PARAM_COUNT; i++) {
        if (strcmp(PARAM_TABLE[i].id, paramId) == 0) {
            return &PARAM_TABLE[i].def;
        }
    }
    return NULL;
}

bool ParamRegistryGetDynamic(const char* paramId, float defaultMin, float defaultMax, ParamDef* outDef)
{
    // Check static table first
    const ParamDef* staticDef = ParamRegistryGet(paramId);
    if (staticDef != NULL) {
        *outDef = *staticDef;
        return true;
    }

    // Accept drawable.* params with provided defaults
    if (strncmp(paramId, "drawable.", 9) == 0) {
        outDef->min = defaultMin;
        outDef->max = defaultMax;
        return true;
    }

    return false;
}
