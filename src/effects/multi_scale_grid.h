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
  float scrollSpeed = 0.005f;  // Base drift rate (multiplied by time in shader)
  float warpAmount = 0.5f;     // Sine distortion amplitude on grid lines
  float edgeContrast = 0.2f;   // Cell boundary darkness
  float edgePower = 3.0f;      // Edge sharpness exponent
  float glowThreshold = 0.55f; // Brightness cutoff for glow (normalized space)
  float glowAmount = 2.0f;     // Glow intensity multiplier
  int glowMode = 0;            // 0 = hard (squared), 1 = soft (linear)
};

typedef struct MultiScaleGridEffect {
  Shader shader;
  int scale1Loc;
  int scale2Loc;
  int scale3Loc;
  int scrollSpeedLoc;
  int warpAmountLoc;
  int edgeContrastLoc;
  int edgePowerLoc;
  int glowThresholdLoc;
  int glowAmountLoc;
  int glowModeLoc;
  int timeLoc;
} MultiScaleGridEffect;

bool MultiScaleGridEffectInit(MultiScaleGridEffect *e);
void MultiScaleGridEffectSetup(MultiScaleGridEffect *e,
                               const MultiScaleGridConfig *cfg, float deltaTime,
                               float transformTime);
void MultiScaleGridEffectUninit(MultiScaleGridEffect *e);
MultiScaleGridConfig MultiScaleGridConfigDefault(void);
void MultiScaleGridRegisterParams(MultiScaleGridConfig *cfg);

#endif // MULTI_SCALE_GRID_H
