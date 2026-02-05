#ifndef MODULATION_ENGINE_H
#define MODULATION_ENGINE_H

#include "mod_sources.h"
#include <stdbool.h>

typedef enum {
  MOD_CURVE_LINEAR = 0,
  MOD_CURVE_EASE_IN,
  MOD_CURVE_EASE_OUT,
  MOD_CURVE_EASE_IN_OUT,
  MOD_CURVE_SPRING,
  MOD_CURVE_ELASTIC,
  MOD_CURVE_BOUNCE,
  MOD_CURVE_COUNT
} ModCurve;

typedef struct {
  char paramId[64];
  int source;   // ModSource enum
  float amount; // -1.0 to +1.0, multiplied by (max-min)
  int curve;    // ModCurve enum
} ModRoute;

void ModEngineInit(void);
void ModEngineUninit(void);

void ModEngineRegisterParam(const char *paramId, float *ptr, float min,
                            float max);
void ModEngineSetRoute(const char *paramId, const ModRoute *route);
void ModEngineRemoveRoute(const char *paramId);
void ModEngineRemoveRoutesMatching(const char *prefix);
void ModEngineRemoveParamsMatching(const char *prefix);
bool ModEngineHasRoute(const char *paramId);
bool ModEngineGetRoute(const char *paramId, ModRoute *outRoute);

void ModEngineUpdate(float dt, const ModSources *sources);

float ModEngineGetOffset(const char *paramId);
float ModEngineGetBase(const char *paramId);
bool ModEngineGetParamBounds(const char *paramId, float *outMin, float *outMax);
void ModEngineSetBase(const char *paramId, float base);

// Route iteration for serialization
int ModEngineGetRouteCount(void);
bool ModEngineGetRouteByIndex(int index, ModRoute *outRoute);
void ModEngineClearRoutes(void);

// Temporarily write base values to all params (for preset saving)
void ModEngineWriteBaseValues(void);

// Read current param values into base (call after preset load)
void ModEngineSyncBases(void);

#endif // MODULATION_ENGINE_H
