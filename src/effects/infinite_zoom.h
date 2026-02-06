#ifndef INFINITE_ZOOM_H
#define INFINITE_ZOOM_H

#include "raylib.h"
#include <stdbool.h>

// Infinite Zoom with spiral rotation
// Tiles scaled layers to simulate continuous forward zoom. spiralAngle offsets
// the base rotation; spiralTwist adds per-layer cumulative rotation for a
// corkscrew trajectory.
struct InfiniteZoomConfig {
  bool enabled = false;
  float speed = 1.0f;       // Zoom animation rate (radians/second)
  float zoomDepth = 3.0f;   // Depth multiplier per zoom cycle (1.0-10.0)
  int layers = 6;           // Number of blended zoom layers (1-16)
  float spiralAngle = 0.0f; // Base rotation offset in radians (+-pi)
  float spiralTwist = 0.0f; // Per-layer rotation in radians (+-pi)
  float layerRotate = 0.0f; // Fixed rotation per layer index in radians (+-pi)
};

typedef struct InfiniteZoomEffect {
  Shader shader;
  int timeLoc;
  int zoomDepthLoc;
  int layersLoc;
  int spiralAngleLoc;
  int spiralTwistLoc;
  int layerRotateLoc;
  int resolutionLoc;
  float time; // Animation accumulator
} InfiniteZoomEffect;

// Returns true on success, false if shader fails to load
bool InfiniteZoomEffectInit(InfiniteZoomEffect *e);

// Accumulates time, sets all uniforms
void InfiniteZoomEffectSetup(InfiniteZoomEffect *e,
                             const InfiniteZoomConfig *cfg, float deltaTime);

// Unloads shader
void InfiniteZoomEffectUninit(InfiniteZoomEffect *e);

// Returns default config
InfiniteZoomConfig InfiniteZoomConfigDefault(void);

// Registers modulatable params with the modulation engine
void InfiniteZoomRegisterParams(InfiniteZoomConfig *cfg);

#endif // INFINITE_ZOOM_H
