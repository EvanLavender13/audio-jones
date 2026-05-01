#ifndef MOBIUS_H
#define MOBIUS_H

#include "raylib.h"
#include <stdbool.h>

#include "config/dual_lissajous_config.h"

struct PostEffect;

// Mobius transformation: conformal mapping with animated control points
struct MobiusConfig {
  bool enabled = false;
  float point1X = 0.3f;
  float point1Y = 0.5f;
  float point2X = 0.7f;
  float point2Y = 0.5f;
  float spiralTightness = 0.5f; // Log-radial winding (0.0-2.0)
  float zoomFactor = -0.5f;     // Log-radial zoom (-2.0-2.0)
  float armCount = 5.0f;        // Spiral arm count (1.0-12.0)
  float spiralOffset = 0.5f;    // Post-Mobius center shift (-1.0-1.0)
  float speed = 1.0f;
  DualLissajousConfig point1Lissajous;
  DualLissajousConfig point2Lissajous;
};

#define MOBIUS_CONFIG_FIELDS                                                   \
  enabled, point1X, point1Y, point2X, point2Y, spiralTightness, zoomFactor,    \
      armCount, spiralOffset, speed, point1Lissajous, point2Lissajous

typedef struct MobiusEffect {
  Shader shader;
  int timeLoc;
  int point1Loc;
  int point2Loc;
  int spiralTightnessLoc;
  int zoomFactorLoc;
  int armCountLoc;
  int spiralOffsetLoc;
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
void MobiusEffectUninit(const MobiusEffect *e);

// Registers modulatable params with the modulation engine
void MobiusRegisterParams(MobiusConfig *cfg);

MobiusEffect *GetMobiusEffect(PostEffect *pe);

#endif // MOBIUS_H
