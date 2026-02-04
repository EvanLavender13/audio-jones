#ifndef CORRIDOR_WARP_H
#define CORRIDOR_WARP_H

#include "raylib.h"
#include <stdbool.h>

enum CorridorWarpMode {
  CORRIDOR_WARP_FLOOR = 0,
  CORRIDOR_WARP_CEILING = 1,
  CORRIDOR_WARP_CORRIDOR = 2
};

struct CorridorWarpConfig {
  bool enabled = false;
  float horizon = 0.5f;
  float perspectiveStrength = 1.0f;
  int mode = CORRIDOR_WARP_CORRIDOR;
  float viewRotationSpeed = 0.0f;
  float planeRotationSpeed = 0.0f;
  float scale = 2.0f;
  float scrollSpeed = 0.5f;
  float strafeSpeed = 0.0f;
  float fogStrength = 1.0f;
};

typedef struct CorridorWarpEffect {
  Shader shader;
  int resolutionLoc;
  int horizonLoc;
  int perspectiveStrengthLoc;
  int modeLoc;
  int viewRotationLoc;
  int planeRotationLoc;
  int scaleLoc;
  int scrollOffsetLoc;
  int strafeOffsetLoc;
  int fogStrengthLoc;
  float viewRotation;
  float planeRotation;
  float scrollOffset;
  float strafeOffset;
} CorridorWarpEffect;

// Returns true on success, false if shader fails to load
bool CorridorWarpEffectInit(CorridorWarpEffect *e);

// Accumulates state values, sets resolution and all uniforms
void CorridorWarpEffectSetup(CorridorWarpEffect *e,
                             const CorridorWarpConfig *cfg, float deltaTime,
                             int screenWidth, int screenHeight);

// Unloads shader
void CorridorWarpEffectUninit(CorridorWarpEffect *e);

// Returns default config
CorridorWarpConfig CorridorWarpConfigDefault(void);

#endif // CORRIDOR_WARP_H
