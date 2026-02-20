#include "param_registry.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "modulation_engine.h"
#include <stddef.h>
#include <string.h>

typedef struct ParamEntry {
  const char *id;
  ParamDef def;
  size_t offset;
} ParamEntry;

static const ParamEntry PARAM_TABLE[] = {
    {"effects.blurScale", {0.0f, 10.0f}, offsetof(EffectConfig, blurScale)},
    {"effects.chromaticOffset",
     {0.0f, 50.0f},
     offsetof(EffectConfig, chromaticOffset)},
    {"effects.motionScale", {0.01f, 1.0f}, offsetof(EffectConfig, motionScale)},
    {"flowField.zoomBase",
     {0.98f, 1.02f},
     offsetof(EffectConfig, flowField.zoomBase)},
    {"flowField.zoomRadial",
     {-0.02f, 0.02f},
     offsetof(EffectConfig, flowField.zoomRadial)},
    {"flowField.rotationSpeed",
     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX},
     offsetof(EffectConfig, flowField.rotationSpeed)},
    {"flowField.rotationSpeedRadial",
     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX},
     offsetof(EffectConfig, flowField.rotationSpeedRadial)},
    {"flowField.dxBase",
     {-0.02f, 0.02f},
     offsetof(EffectConfig, flowField.dxBase)},
    {"flowField.dxRadial",
     {-0.02f, 0.02f},
     offsetof(EffectConfig, flowField.dxRadial)},
    {"flowField.dyBase",
     {-0.02f, 0.02f},
     offsetof(EffectConfig, flowField.dyBase)},
    {"flowField.dyRadial",
     {-0.02f, 0.02f},
     offsetof(EffectConfig, flowField.dyRadial)},
    {"flowField.cx", {0.0f, 1.0f}, offsetof(EffectConfig, flowField.cx)},
    {"flowField.cy", {0.0f, 1.0f}, offsetof(EffectConfig, flowField.cy)},
    {"flowField.sx", {0.9f, 1.1f}, offsetof(EffectConfig, flowField.sx)},
    {"flowField.sy", {0.9f, 1.1f}, offsetof(EffectConfig, flowField.sy)},
    {"flowField.zoomAngular",
     {-0.1f, 0.1f},
     offsetof(EffectConfig, flowField.zoomAngular)},
    {"flowField.rotAngular",
     {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX},
     offsetof(EffectConfig, flowField.rotAngular)},
    {"flowField.dxAngular",
     {-0.02f, 0.02f},
     offsetof(EffectConfig, flowField.dxAngular)},
    {"flowField.dyAngular",
     {-0.02f, 0.02f},
     offsetof(EffectConfig, flowField.dyAngular)},
    {"proceduralWarp.warp",
     {0.0f, 2.0f},
     offsetof(EffectConfig, proceduralWarp.warp)},
    {"proceduralWarp.warpSpeed",
     {0.1f, 2.0f},
     offsetof(EffectConfig, proceduralWarp.warpSpeed)},
    {"proceduralWarp.warpScale",
     {0.1f, 100.0f},
     offsetof(EffectConfig, proceduralWarp.warpScale)},
    {"feedbackFlow.strength",
     {0.0f, 20.0f},
     offsetof(EffectConfig, feedbackFlow.strength)},
    {"feedbackFlow.flowAngle",
     {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX},
     offsetof(EffectConfig, feedbackFlow.flowAngle)},
    {"feedbackFlow.scale",
     {1.0f, 5.0f},
     offsetof(EffectConfig, feedbackFlow.scale)},
    {"feedbackFlow.threshold",
     {0.0f, 0.1f},
     offsetof(EffectConfig, feedbackFlow.threshold)},
};

static const int PARAM_COUNT = sizeof(PARAM_TABLE) / sizeof(PARAM_TABLE[0]);

// Drawable field bounds: matched by field name in "drawable.<id>.<field>"
static const ParamEntry DRAWABLE_FIELD_TABLE[] = {
    {"x", {-1.0f, 2.0f}, 0},
    {"y", {-1.0f, 2.0f}, 0},
    {"rotationSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}, 0},
    {"rotationAngle", {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}, 0},
    {"texAngle", {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}, 0},
    {"texMotionScale", {0.01f, 1.0f}, 0},
    {"width", {0.01f, 2.0f}, 0},
    {"height", {0.01f, 2.0f}, 0},
    {"radius", {0.05f, 1.0f}, 0},
    {"amplitudeScale", {0.05f, 0.5f}, 0},
    {"thickness", {1.0f, 50.0f}, 0},
    {"size", {1.0f, 100.0f}, 0},
    {"smoothness", {0.0f, 100.0f}, 0},
    {"waveformMotionScale", {0.01f, 1.0f}, 0},
    // Parametric trail lissajous fields
    {"lissajous.amplitude", {0.05f, 0.5f}, 0},
    {"lissajous.motionSpeed", {0.1f, 10.0f}, 0},
    // Parametric trail random walk fields
    {"randomWalk.stepSize", {0.001f, 0.1f}, 0},
    {"randomWalk.smoothness", {0.0f, 1.0f}, 0},
    {"gateFreq", {0.0f, 20.0f}, 0},
    {"strokeThickness", {1.0f, 10.0f}, 0},
    {"colorShift", {0.0f, TWO_PI_F}, 0},
    {"colorShiftSpeed", {-TWO_PI_F, TWO_PI_F}, 0},
    {"innerRadius", {0.05f, 0.4f}, 0},
    {"barHeight", {0.1f, 0.5f}, 0},
    {"barWidth", {0.3f, 1.0f}, 0},
    {"smoothing", {0.0f, 0.95f}, 0},
};

static const int DRAWABLE_FIELD_COUNT =
    sizeof(DRAWABLE_FIELD_TABLE) / sizeof(DRAWABLE_FIELD_TABLE[0]);

void ParamRegistryInit(EffectConfig *effects) {
  for (int i = 0; i < PARAM_COUNT; i++) {
    float *target = (float *)((char *)effects + PARAM_TABLE[i].offset);
    ModEngineRegisterParam(PARAM_TABLE[i].id, target, PARAM_TABLE[i].def.min,
                           PARAM_TABLE[i].def.max);
  }
}

bool ParamRegistryGetDynamic(const char *paramId, ParamDef *outDef) {
  // Check ModEngine hashmap first (O(1) lookup for all registered params)
  float min;
  float max;
  if (ModEngineGetParamBounds(paramId, &min, &max)) {
    outDef->min = min;
    outDef->max = max;
    return true;
  }

  // Match drawable.<id>.<field> pattern
  if (strncmp(paramId, "drawable.", 9) == 0) {
    const char *afterPrefix = paramId + 9;
    const char *dot = strchr(afterPrefix, '.');
    if (dot != NULL) {
      const char *fieldName = dot + 1;
      for (int i = 0; i < DRAWABLE_FIELD_COUNT; i++) {
        if (strcmp(DRAWABLE_FIELD_TABLE[i].id, fieldName) == 0) {
          *outDef = DRAWABLE_FIELD_TABLE[i].def;
          return true;
        }
      }
    }
  }

  // Match lfo<n>.rate pattern (registered separately via
  // ModEngineRegisterParam)
  if (strncmp(paramId, "lfo", 3) == 0 && strstr(paramId, ".rate") != NULL) {
    outDef->min = LFO_RATE_MIN;
    outDef->max = LFO_RATE_MAX;
    return true;
  }

  return false;
}
