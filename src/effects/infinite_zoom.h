#ifndef INFINITE_ZOOM_H
#define INFINITE_ZOOM_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include <stdbool.h>

// Infinite Zoom with spiral rotation
// Tiles scaled layers to simulate continuous forward zoom. spiralAngle offsets
// the base rotation; spiralTwist adds per-layer cumulative rotation for a
// corkscrew trajectory.
struct InfiniteZoomConfig {
  bool enabled = false;
  float speed = 1.0f;        // Zoom animation rate (radians/second)
  float zoomDepth = 3.0f;    // Depth multiplier per zoom cycle (1.0-10.0)
  int layers = 6;            // Number of blended zoom layers (1-16)
  float spiralAngle = 0.0f;  // Base rotation offset in radians (+-pi)
  float spiralTwist = 0.0f;  // Per-layer rotation in radians (+-pi)
  float layerRotate = 0.0f;  // Fixed rotation per layer index in radians (+-pi)
  int warpType = 0;          // 0=None, 1=Sine, 2=Noise
  float warpStrength = 0.0f; // Distortion amplitude (0.0-1.0)
  float warpFreq = 3.0f;     // Spatial frequency of warp (1.0-10.0)
  float warpSpeed = 0.5f;    // Warp animation rate (-2.0 to 2.0)
  float centerX = 0.5f;      // Zoom focal point X (0.0-1.0)
  float centerY = 0.5f;      // Zoom focal point Y (0.0-1.0)
  DualLissajousConfig centerLissajous = {.amplitude = 0.0f};
  float offsetX = 0.0f; // Per-layer X drift (-0.1 to 0.1)
  float offsetY = 0.0f; // Per-layer Y drift (-0.1 to 0.1)
  DualLissajousConfig offsetLissajous = {.amplitude = 0.0f};
  int blendMode = 0; // 0=Weighted Avg, 1=Additive, 2=Screen
};

#define INFINITE_ZOOM_CONFIG_FIELDS                                            \
  enabled, speed, zoomDepth, layers, spiralAngle, spiralTwist, layerRotate,    \
      warpType, warpStrength, warpFreq, warpSpeed, centerX, centerY,           \
      centerLissajous, offsetX, offsetY, offsetLissajous, blendMode

typedef struct InfiniteZoomEffect {
  Shader shader;
  int timeLoc;
  int zoomDepthLoc;
  int layersLoc;
  int spiralAngleLoc;
  int spiralTwistLoc;
  int layerRotateLoc;
  int resolutionLoc;
  int centerLoc;
  int offsetLoc;
  int warpTypeLoc;
  int warpStrengthLoc;
  int warpFreqLoc;
  int warpTimeLoc;
  int blendModeLoc;
  float time;     // Animation accumulator
  float warpTime; // Warp animation accumulator
} InfiniteZoomEffect;

// Returns true on success, false if shader fails to load
bool InfiniteZoomEffectInit(InfiniteZoomEffect *e);

// Accumulates time, sets all uniforms
void InfiniteZoomEffectSetup(InfiniteZoomEffect *e, InfiniteZoomConfig *cfg,
                             float deltaTime);

// Unloads shader
void InfiniteZoomEffectUninit(InfiniteZoomEffect *e);

// Returns default config
InfiniteZoomConfig InfiniteZoomConfigDefault(void);

// Registers modulatable params with the modulation engine
void InfiniteZoomRegisterParams(InfiniteZoomConfig *cfg);

#endif // INFINITE_ZOOM_H
