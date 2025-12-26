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
};

static const int PARAM_COUNT = sizeof(PARAM_TABLE) / sizeof(PARAM_TABLE[0]);

void ParamRegistryInit(EffectConfig* effects)
{
    ModEngineRegisterParam("physarum.sensorDistance", &effects->physarum.sensorDistance,
                           PARAM_TABLE[0].def.min, PARAM_TABLE[0].def.max);
    ModEngineRegisterParam("physarum.sensorAngle", &effects->physarum.sensorAngle,
                           PARAM_TABLE[1].def.min, PARAM_TABLE[1].def.max);
    ModEngineRegisterParam("physarum.turningAngle", &effects->physarum.turningAngle,
                           PARAM_TABLE[2].def.min, PARAM_TABLE[2].def.max);
    ModEngineRegisterParam("physarum.stepSize", &effects->physarum.stepSize,
                           PARAM_TABLE[3].def.min, PARAM_TABLE[3].def.max);
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
