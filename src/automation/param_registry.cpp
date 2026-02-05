#include "param_registry.h"
#include "config/effect_config.h"
#include "modulation_engine.h"
#include "ui/ui_units.h"
#include <stddef.h>
#include <string.h>

typedef struct ParamEntry {
  const char *id;
  ParamDef def;
  size_t offset;
} ParamEntry;

static const ParamEntry PARAM_TABLE[] = {
    {"physarum.sensorDistance",
     {1.0f, 100.0f},
     offsetof(EffectConfig, physarum.sensorDistance)},
    {"physarum.sensorDistanceVariance",
     {0.0f, 20.0f},
     offsetof(EffectConfig, physarum.sensorDistanceVariance)},
    {"physarum.sensorAngle",
     {0.0f, 6.28f},
     offsetof(EffectConfig, physarum.sensorAngle)},
    {"physarum.turningAngle",
     {0.0f, 6.28f},
     offsetof(EffectConfig, physarum.turningAngle)},
    {"physarum.stepSize",
     {0.1f, 100.0f},
     offsetof(EffectConfig, physarum.stepSize)},
    {"physarum.levyAlpha",
     {0.1f, 3.0f},
     offsetof(EffectConfig, physarum.levyAlpha)},
    {"physarum.densityResponse",
     {0.1f, 5.0f},
     offsetof(EffectConfig, physarum.densityResponse)},
    {"physarum.cauchyScale",
     {0.1f, 2.0f},
     offsetof(EffectConfig, physarum.cauchyScale)},
    {"physarum.expScale",
     {0.1f, 3.0f},
     offsetof(EffectConfig, physarum.expScale)},
    {"physarum.gaussianVariance",
     {0.0f, 1.0f},
     offsetof(EffectConfig, physarum.gaussianVariance)},
    {"physarum.sprintFactor",
     {0.0f, 5.0f},
     offsetof(EffectConfig, physarum.sprintFactor)},
    {"physarum.gradientBoost",
     {0.0f, 10.0f},
     offsetof(EffectConfig, physarum.gradientBoost)},
    {"physarum.repulsionStrength",
     {0.0f, 1.0f},
     offsetof(EffectConfig, physarum.repulsionStrength)},
    {"physarum.samplingExponent",
     {0.0f, 10.0f},
     offsetof(EffectConfig, physarum.samplingExponent)},
    {"physarum.gravityStrength",
     {0.0f, 1.0f},
     offsetof(EffectConfig, physarum.gravityStrength)},
    {"physarum.orbitOffset",
     {0.0f, 1.0f},
     offsetof(EffectConfig, physarum.orbitOffset)},
    {"physarum.lissajous.amplitude",
     {0.0f, 0.5f},
     offsetof(EffectConfig, physarum.lissajous.amplitude)},
    {"physarum.lissajous.motionSpeed",
     {0.0f, 10.0f},
     offsetof(EffectConfig, physarum.lissajous.motionSpeed)},
    {"physarum.attractorBaseRadius",
     {0.1f, 0.5f},
     offsetof(EffectConfig, physarum.attractorBaseRadius)},
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
    {"attractorFlow.rotationSpeedX",
     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX},
     offsetof(EffectConfig, attractorFlow.rotationSpeedX)},
    {"attractorFlow.rotationSpeedY",
     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX},
     offsetof(EffectConfig, attractorFlow.rotationSpeedY)},
    {"attractorFlow.rotationSpeedZ",
     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX},
     offsetof(EffectConfig, attractorFlow.rotationSpeedZ)},
    {"attractorFlow.rotationAngleX",
     {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX},
     offsetof(EffectConfig, attractorFlow.rotationAngleX)},
    {"attractorFlow.rotationAngleY",
     {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX},
     offsetof(EffectConfig, attractorFlow.rotationAngleY)},
    {"attractorFlow.rotationAngleZ",
     {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX},
     offsetof(EffectConfig, attractorFlow.rotationAngleZ)},
    {"particleLife.rotationSpeedX",
     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX},
     offsetof(EffectConfig, particleLife.rotationSpeedX)},
    {"particleLife.rotationSpeedY",
     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX},
     offsetof(EffectConfig, particleLife.rotationSpeedY)},
    {"particleLife.rotationSpeedZ",
     {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX},
     offsetof(EffectConfig, particleLife.rotationSpeedZ)},
    {"particleLife.rotationAngleX",
     {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX},
     offsetof(EffectConfig, particleLife.rotationAngleX)},
    {"particleLife.rotationAngleY",
     {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX},
     offsetof(EffectConfig, particleLife.rotationAngleY)},
    {"particleLife.rotationAngleZ",
     {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX},
     offsetof(EffectConfig, particleLife.rotationAngleZ)},
    {"particleLife.rMax",
     {0.05f, 0.5f},
     offsetof(EffectConfig, particleLife.rMax)},
    {"particleLife.forceFactor",
     {0.01f, 2.0f},
     offsetof(EffectConfig, particleLife.forceFactor)},
    {"particleLife.momentum",
     {0.1f, 0.99f},
     offsetof(EffectConfig, particleLife.momentum)},
    {"particleLife.beta",
     {0.1f, 0.9f},
     offsetof(EffectConfig, particleLife.beta)},
    {"particleLife.evolutionSpeed",
     {0.0f, 5.0f},
     offsetof(EffectConfig, particleLife.evolutionSpeed)},
    {"boids.cohesionWeight",
     {0.0f, 2.0f},
     offsetof(EffectConfig, boids.cohesionWeight)},
    {"boids.separationWeight",
     {0.0f, 2.0f},
     offsetof(EffectConfig, boids.separationWeight)},
    {"boids.alignmentWeight",
     {0.0f, 2.0f},
     offsetof(EffectConfig, boids.alignmentWeight)},
    {"curlFlow.respawnProbability",
     {0.0f, 0.1f},
     offsetof(EffectConfig, curlFlow.respawnProbability)},
    {"curlAdvection.advectionCurl",
     {0.0f, 1.0f},
     offsetof(EffectConfig, curlAdvection.advectionCurl)},
    {"curlAdvection.curlScale",
     {-4.0f, 4.0f},
     offsetof(EffectConfig, curlAdvection.curlScale)},
    {"curlAdvection.selfAmp",
     {0.5f, 2.0f},
     offsetof(EffectConfig, curlAdvection.selfAmp)},
    {"curlAdvection.laplacianScale",
     {0.0f, 0.2f},
     offsetof(EffectConfig, curlAdvection.laplacianScale)},
    {"curlAdvection.pressureScale",
     {-4.0f, 4.0f},
     offsetof(EffectConfig, curlAdvection.pressureScale)},
    {"curlAdvection.divergenceScale",
     {-1.0f, 1.0f},
     offsetof(EffectConfig, curlAdvection.divergenceScale)},
    {"curlAdvection.divergenceUpdate",
     {-0.1f, 0.1f},
     offsetof(EffectConfig, curlAdvection.divergenceUpdate)},
    {"curlAdvection.divergenceSmoothing",
     {0.0f, 0.5f},
     offsetof(EffectConfig, curlAdvection.divergenceSmoothing)},
    {"curlAdvection.updateSmoothing",
     {0.25f, 0.9f},
     offsetof(EffectConfig, curlAdvection.updateSmoothing)},
    {"curlAdvection.injectionIntensity",
     {0.0f, 1.0f},
     offsetof(EffectConfig, curlAdvection.injectionIntensity)},
    {"curlAdvection.injectionThreshold",
     {0.0f, 1.0f},
     offsetof(EffectConfig, curlAdvection.injectionThreshold)},
    {"curlAdvection.decayHalfLife",
     {0.1f, 5.0f},
     offsetof(EffectConfig, curlAdvection.decayHalfLife)},
    {"curlAdvection.boostIntensity",
     {0.0f, 5.0f},
     offsetof(EffectConfig, curlAdvection.boostIntensity)},
    {"cymatics.waveScale",
     {1.0f, 50.0f},
     offsetof(EffectConfig, cymatics.waveScale)},
    {"cymatics.falloff",
     {0.0f, 5.0f},
     offsetof(EffectConfig, cymatics.falloff)},
    {"cymatics.visualGain",
     {0.5f, 5.0f},
     offsetof(EffectConfig, cymatics.visualGain)},
    {"cymatics.boostIntensity",
     {0.0f, 5.0f},
     offsetof(EffectConfig, cymatics.boostIntensity)},
    {"cymatics.baseRadius",
     {0.0f, 0.5f},
     offsetof(EffectConfig, cymatics.baseRadius)},
    {"cymatics.reflectionGain",
     {0.0f, 1.0f},
     offsetof(EffectConfig, cymatics.reflectionGain)},
    {"cymatics.lissajous.amplitude",
     {0.0f, 0.5f},
     offsetof(EffectConfig, cymatics.lissajous.amplitude)},
    {"cymatics.lissajous.motionSpeed",
     {0.0f, 5.0f},
     offsetof(EffectConfig, cymatics.lissajous.motionSpeed)},
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
    {"gateFreq", {0.0f, 20.0f}, 0},
    {"strokeThickness", {1.0f, 10.0f}, 0},
    {"colorShift", {0.0f, 2.0f * PI}, 0},
    {"colorShiftSpeed", {-2.0f * PI, 2.0f * PI}, 0},
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
  float min, max;
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
