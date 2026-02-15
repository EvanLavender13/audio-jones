// Multi-Scale Grid: Layered grid overlay with drift, warp, and edge glow

#ifndef MULTI_SCALE_GRID_H
#define MULTI_SCALE_GRID_H

#include "raylib.h"
#include <stdbool.h>

struct MultiScaleGridConfig {
  bool enabled = false;
  float scale1 = 10.0f;        // Coarse grid density
  float scale2 = 25.0f;        // Medium grid density
  float scale3 = 50.0f;        // Fine grid density
  float warpAmount = 0.5f;     // Sine distortion amplitude on grid lines
  float edgeContrast = 0.2f;   // Cell boundary darkness
  float edgePower = 3.0f;      // Edge sharpness exponent
  float glowThreshold = 0.55f; // Brightness cutoff for glow (normalized space)
  float glowAmount = 2.0f;     // Glow intensity multiplier
  float cellVariation = 0.3f;  // Per-cell brightness spread around 1.0
  int glowMode = 0;            // 0 = hard (squared), 1 = soft (linear)
};

#define MULTI_SCALE_GRID_CONFIG_FIELDS                                         \
  enabled, scale1, scale2, scale3, warpAmount, edgeContrast, edgePower,        \
      glowThreshold, glowAmount, cellVariation, glowMode

typedef struct MultiScaleGridEffect {
  Shader shader;
  int scale1Loc;
  int scale2Loc;
  int scale3Loc;
  int warpAmountLoc;
  int edgeContrastLoc;
  int edgePowerLoc;
  int glowThresholdLoc;
  int glowAmountLoc;
  int glowModeLoc;
  int cellVariationLoc;
} MultiScaleGridEffect;

bool MultiScaleGridEffectInit(MultiScaleGridEffect *e);
void MultiScaleGridEffectSetup(MultiScaleGridEffect *e,
                               const MultiScaleGridConfig *cfg);
void MultiScaleGridEffectUninit(MultiScaleGridEffect *e);
MultiScaleGridConfig MultiScaleGridConfigDefault(void);
void MultiScaleGridRegisterParams(MultiScaleGridConfig *cfg);

#endif // MULTI_SCALE_GRID_H
