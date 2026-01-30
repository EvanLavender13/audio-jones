#ifndef PARAM_REGISTRY_H
#define PARAM_REGISTRY_H

#include <stdbool.h>

struct EffectConfig;

typedef struct ParamDef {
  float min;
  float max;
} ParamDef;

void ParamRegistryInit(EffectConfig *effects);
const ParamDef *ParamRegistryGet(const char *paramId);

// Look up param bounds: checks static table, then drawable field patterns
bool ParamRegistryGetDynamic(const char *paramId, ParamDef *outDef);

#endif // PARAM_REGISTRY_H
