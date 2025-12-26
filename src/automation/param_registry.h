#ifndef PARAM_REGISTRY_H
#define PARAM_REGISTRY_H

struct EffectConfig;

typedef struct ParamDef {
    float min;
    float max;
} ParamDef;

void ParamRegistryInit(EffectConfig* effects);
const ParamDef* ParamRegistryGet(const char* paramId);

#endif // PARAM_REGISTRY_H
