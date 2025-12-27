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
    {"feedback.rotation",       {-0.1f, 0.1f}},
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
        &effects->feedbackRotation,
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
