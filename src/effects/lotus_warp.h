// Lotus Warp - log-polar conformal coordinate warp with cellular sub-effects

#ifndef LOTUS_WARP_H
#define LOTUS_WARP_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

struct LotusWarpConfig {
  bool enabled = false;
  float scale = 3.0f;         // Cell density (0.5-10.0)
  float zoomSpeed = 0.3f;     // Radial zoom rate, rad/s (-PI_F to PI_F)
  float spinSpeed = 0.0f;     // Angular rotation rate, rad/s (-PI_F to PI_F)
  float edgeFalloff = 0.3f;   // Edge softness (0.01-1.0)
  float isoFrequency = 10.0f; // Ring density for iso effects (1.0-30.0)
  int mode = 0;               // CellMode index (0-8)
  float intensity = 0.5f;     // Sub-effect strength (0.0-1.0)
};

#define LOTUS_WARP_CONFIG_FIELDS                                               \
  enabled, scale, zoomSpeed, spinSpeed, edgeFalloff, isoFrequency, mode,       \
      intensity

typedef struct LotusWarpEffect {
  Shader shader;
  int resolutionLoc;
  int scaleLoc;
  int zoomPhaseLoc;
  int spinPhaseLoc;
  int edgeFalloffLoc;
  int isoFrequencyLoc;
  int modeLoc;
  int intensityLoc;
  float zoomPhase; // CPU-accumulated zoom offset
  float spinPhase; // CPU-accumulated spin offset
} LotusWarpEffect;

bool LotusWarpEffectInit(LotusWarpEffect *e);
void LotusWarpEffectSetup(LotusWarpEffect *e, const LotusWarpConfig *cfg,
                          float deltaTime);
void LotusWarpEffectUninit(const LotusWarpEffect *e);
void LotusWarpRegisterParams(LotusWarpConfig *cfg);

LotusWarpEffect *GetLotusWarpEffect(PostEffect *pe);

#endif // LOTUS_WARP_H
