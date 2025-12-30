#ifndef PARAM_REGISTRY_H
#define PARAM_REGISTRY_H

#include <stdbool.h>

struct EffectConfig;

typedef struct ParamDef {
    float min;
    float max;
} ParamDef;

void ParamRegistryInit(EffectConfig* effects);
const ParamDef* ParamRegistryGet(const char* paramId);

// Look up param bounds: checks static table first, then accepts dynamic params (drawable.*)
bool ParamRegistryGetDynamic(const char* paramId, float defaultMin, float defaultMax, ParamDef* outDef);

#endif // PARAM_REGISTRY_H
