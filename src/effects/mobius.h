#ifndef MOBIUS_H
#define MOBIUS_H

#include "raylib.h"
#include <stdbool.h>

#include "config/dual_lissajous_config.h"

// Mobius transformation: conformal mapping with animated control points
struct MobiusConfig {
  bool enabled = false;
  float point1X = 0.3f;
  float point1Y = 0.5f;
  float point2X = 0.7f;
  float point2Y = 0.5f;
  float spiralTightness = 0.0f;
  float zoomFactor = 0.0f;
  float speed = 1.0f;
  DualLissajousConfig point1Lissajous;
  DualLissajousConfig point2Lissajous;
};

typedef struct MobiusEffect {
  Shader shader;
  int timeLoc;
  int point1Loc;
  int point2Loc;
  int spiralTightnessLoc;
  int zoomFactorLoc;
  float time;
  float currentPoint1[2];
  float currentPoint2[2];
} MobiusEffect;

// Returns true on success, false if shader fails to load
bool MobiusEffectInit(MobiusEffect *e);

// Accumulates time, computes Lissajous points, sets all uniforms
// Non-const config: DualLissajousUpdate mutates internal phase state
void MobiusEffectSetup(MobiusEffect *e, MobiusConfig *cfg, float deltaTime);

// Unloads shader
void MobiusEffectUninit(MobiusEffect *e);

// Returns default config
MobiusConfig MobiusConfigDefault(void);

#endif // MOBIUS_H
