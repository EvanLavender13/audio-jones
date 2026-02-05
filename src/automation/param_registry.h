#ifndef PARAM_REGISTRY_H
#define PARAM_REGISTRY_H

#include <stdbool.h>

struct EffectConfig;

typedef struct ParamDef {
  float min;
  float max;
} ParamDef;

void ParamRegistryInit(EffectConfig *effects);

// Look up param bounds: checks ModEngine hashmap, then drawable field patterns
bool ParamRegistryGetDynamic(const char *paramId, ParamDef *outDef);

#endif // PARAM_REGISTRY_H
