// LEGO bricks effect module
// 3D-styled brick pixelation with studs and variable sizing

#ifndef LEGO_BRICKS_H
#define LEGO_BRICKS_H

#include "raylib.h"
#include <stdbool.h>

struct LegoBricksConfig {
  bool enabled = false;
  float brickScale = 0.04f;    // Brick size relative to screen (0.01-0.2)
  float studHeight = 0.5f;     // Stud highlight intensity (0.0-1.0)
  float edgeShadow = 0.2f;     // Edge shadow darkness (0.0-1.0)
  float colorThreshold = 0.1f; // Color similarity for merging (0.0-0.5)
  int maxBrickSize = 2;        // Largest brick dimension (1-4)
  float lightAngle = 0.7854f;  // Light direction in radians (default 45 deg)
};

typedef struct LegoBricksEffect {
  Shader shader;
  int resolutionLoc;
  int brickScaleLoc;
  int studHeightLoc;
  int edgeShadowLoc;
  int colorThresholdLoc;
  int maxBrickSizeLoc;
  int lightAngleLoc;
} LegoBricksEffect;

// Returns true on success, false if shader fails to load
bool LegoBricksEffectInit(LegoBricksEffect *e);

// Sets all uniforms
void LegoBricksEffectSetup(LegoBricksEffect *e, const LegoBricksConfig *cfg);

// Unloads shader
void LegoBricksEffectUninit(LegoBricksEffect *e);

// Returns default config
LegoBricksConfig LegoBricksConfigDefault(void);

#endif // LEGO_BRICKS_H
