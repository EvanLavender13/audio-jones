#ifndef LENS_SPACE_H
#define LENS_SPACE_H

#include "raylib.h"
#include <stdbool.h>

// Lens Space — warps image through a spherical lens with p/q symmetry
// reflections, creating kaleidoscopic mirrored geometry inside a bounding
// sphere.
struct LensSpaceConfig {
  bool enabled = false;
  float centerX = 0.5f;         // Warp center X (0.0-1.0)
  float centerY = 0.5f;         // Warp center Y (0.0-1.0)
  float p = 5.0f;               // Symmetry order (2.0-12.0)
  float q = 2.0f;               // Rotation fraction (1.0-11.0)
  float sphereOffsetX = 0.0f;   // Sphere center X offset (-0.5 to 0.5)
  float sphereOffsetY = 0.0f;   // Sphere center Y offset (-0.5 to 0.5)
  float sphereRadius = 0.3f;    // Central mirror sphere size (0.05-0.8)
  float boundaryRadius = 1.0f;  // Lens space boundary radius (0.5-2.0)
  float rotationSpeed = 0.5f;   // Camera rotation rate rad/s
                                // (-ROTATION_SPEED_MAX..+ROTATION_SPEED_MAX)
  float maxReflections = 12.0f; // Reflection depth (2.0-20.0)
  float dimming = 0.067f;       // Per-reflection brightness decay (0.01-0.15)
  float zoom = 1.0f;            // Ray spread / FOV (0.5-3.0)
  float projScale = 0.4f;       // UV projection strength (0.1-1.0)
};

#define LENS_SPACE_CONFIG_FIELDS                                               \
  enabled, centerX, centerY, sphereOffsetX, sphereOffsetY, p, q, sphereRadius, \
      boundaryRadius, rotationSpeed, maxReflections, dimming, zoom, projScale

typedef struct LensSpaceEffect {
  Shader shader;
  int resolutionLoc;
  int centerLoc;
  int sphereOffsetLoc;
  int pLoc;
  int qLoc;
  int sphereRadiusLoc;
  int boundaryRadiusLoc;
  int rotAngleLoc;
  int maxReflectionsLoc;
  int dimmingLoc;
  int zoomLoc;
  int projScaleLoc;
  float rotAngle; // CPU-accumulated rotation
} LensSpaceEffect;

// Returns true on success, false if shader fails to load
bool LensSpaceEffectInit(LensSpaceEffect *e);

// Accumulates rotation, sets resolution and all uniforms
void LensSpaceEffectSetup(LensSpaceEffect *e, const LensSpaceConfig *cfg,
                          float deltaTime, int screenWidth, int screenHeight);

// Unloads shader
void LensSpaceEffectUninit(LensSpaceEffect *e);

// Returns default config
LensSpaceConfig LensSpaceConfigDefault(void);

// Registers modulatable params with the modulation engine
void LensSpaceRegisterParams(LensSpaceConfig *cfg);

#endif // LENS_SPACE_H
