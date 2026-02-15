#ifndef SURFACE_WARP_H
#define SURFACE_WARP_H

#include "raylib.h"
#include <stdbool.h>

// Surface Warp with rotation and scroll accumulation
// Creates rolling hill/wave terrain distortion with directional shading.
// Rotation controls warp direction, scroll animates wave movement.
struct SurfaceWarpConfig {
  bool enabled = false;
  float intensity = 0.5f;     // Hill steepness (0.0-2.0)
  float angle = 0.0f;         // Base warp direction (-PI to PI)
  float rotationSpeed = 0.0f; // Direction rotation rate (rad/s)
  float scrollSpeed = 0.5f;   // Wave drift speed (-2.0 to 2.0)
  float depthShade = 0.3f;    // Valley darkening amount (0.0-1.0)
};

#define SURFACE_WARP_CONFIG_FIELDS                                             \
  enabled, intensity, angle, rotationSpeed, scrollSpeed, depthShade

typedef struct SurfaceWarpEffect {
  Shader shader;
  int intensityLoc;
  int angleLoc;
  int rotationLoc;
  int scrollOffsetLoc;
  int depthShadeLoc;
  float rotation;     // Accumulated rotation
  float scrollOffset; // Accumulated scroll
} SurfaceWarpEffect;

// Returns true on success, false if shader fails to load
bool SurfaceWarpEffectInit(SurfaceWarpEffect *e);

// Accumulates rotation and scrollOffset, sets all uniforms
void SurfaceWarpEffectSetup(SurfaceWarpEffect *e, const SurfaceWarpConfig *cfg,
                            float deltaTime);

// Unloads shader
void SurfaceWarpEffectUninit(SurfaceWarpEffect *e);

// Returns default config
SurfaceWarpConfig SurfaceWarpConfigDefault(void);

// Registers modulatable params with the modulation engine
void SurfaceWarpRegisterParams(SurfaceWarpConfig *cfg);

#endif // SURFACE_WARP_H
