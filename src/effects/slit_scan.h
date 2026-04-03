// Slit Scan effect module
// Samples a vertical slit and extrudes it into a perspective corridor or flat
// scroll via ping-pong accumulation

#ifndef SLIT_SCAN_EFFECT_H
#define SLIT_SCAN_EFFECT_H

#include "raylib.h"
#include <stdbool.h>

struct SlitScanConfig {
  bool enabled = false;

  // Slit sampling
  float slitPosition = 0.5f; // Horizontal UV to sample (0.0-1.0)
  float slitWidth = 0.005f;  // Slit width in UV space (0.001-1.0)

  // Mode
  int mode = 0; // 0 = Corridor, 1 = Flat (not modulatable)

  // Corridor dynamics
  float speed = 2.0f;       // Outward advance rate (0.1-10.0)
  float pushAccel = 0.3f;   // Edge push acceleration (0.0-10.0)
  float perspective = 0.3f; // Vertical wall spread at edges (0.0-10.0)
  float fogStrength = 1.0f; // Depth brightness falloff (0.1-5.0)
  float brightness = 1.0f;  // Fresh slit brightness (0.1-3.0)
  float glow = 0.0f;        // Center brightness falloff steepness (0.0-5.0)

  // Rotation (display-time only, NOT inside ping-pong)
  float rotationAngle = 0.0f; // Static rotation radians (-PI..PI)
  float rotationSpeed = 0.0f; // Rotation rate rad/s (-PI..PI)
};

#define SLIT_SCAN_CONFIG_FIELDS                                                \
  enabled, slitPosition, slitWidth, mode, speed, pushAccel, perspective,       \
      fogStrength, brightness, glow, rotationAngle, rotationSpeed

typedef struct SlitScanEffect {
  Shader shader;        // Accumulation (ping-pong)
  Shader displayShader; // Perspective + rotation pass
  RenderTexture2D pingPong[2];
  int readIdx;
  float rotationAccum;

  // Accumulation shader locations
  int resolutionLoc;
  int sceneTextureLoc;
  int slitPositionLoc;
  int speedDtLoc;
  int pushAccelLoc;
  int slitWidthLoc;
  int brightnessLoc;
  int centerLoc;

  // Display shader locations
  int dispResolutionLoc;
  int dispModeLoc;
  int dispRotationLoc;
  int dispPerspectiveLoc;
  int dispFogLoc;
  int dispCenterLoc;
  int dispGlowLoc;
} SlitScanEffect;

typedef struct PostEffect PostEffect;

// Loads shaders, caches uniform locations, allocates ping-pong textures
bool SlitScanEffectInit(SlitScanEffect *e, const SlitScanConfig *cfg, int width,
                        int height);

// Binds scalar uniforms and accumulates rotation state
void SlitScanEffectSetup(SlitScanEffect *e, const SlitScanConfig *cfg,
                         float deltaTime);

// Executes ping-pong accumulation and writes output to pipeline destination
void SlitScanEffectRender(SlitScanEffect *e, const SlitScanConfig *cfg,
                          const PostEffect *pe);

// Unloads ping-pong textures, reallocates at new dimensions
void SlitScanEffectResize(SlitScanEffect *e, int width, int height);

// Unloads shaders and ping-pong textures
void SlitScanEffectUninit(SlitScanEffect *e);

// Registers modulatable params with the modulation engine
void SlitScanRegisterParams(SlitScanConfig *cfg);

#endif // SLIT_SCAN_EFFECT_H
