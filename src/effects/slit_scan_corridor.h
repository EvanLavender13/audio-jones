// Slit Scan Corridor effect module
// Samples a vertical slit and extrudes it into a perspective corridor via
// ping-pong accumulation

#ifndef SLIT_SCAN_CORRIDOR_H
#define SLIT_SCAN_CORRIDOR_H

#include "raylib.h"
#include "render/blend_mode.h"
#include <stdbool.h>

struct SlitScanCorridorConfig {
  bool enabled = false;

  // Slit sampling
  float slitPosition = 0.5f; // Horizontal UV to sample (0.0-1.0)
  float slitWidth = 0.005f;  // Feathering radius (0.001-0.05)

  // Corridor dynamics
  float speed = 2.0f;         // Outward advance rate (0.1-10.0)
  float perspective = 3.0f;   // Foreshortening strength (0.5-8.0)
  float decayHalfLife = 3.0f; // Trail brightness half-life seconds (0.1-10.0)
  float brightness = 1.0f;    // Fresh slit brightness (0.1-3.0)

  // Rotation (display-time only, NOT inside ping-pong)
  float rotationAngle = 0.0f; // Static rotation radians (-PI..PI)
  float rotationSpeed = 0.0f; // Rotation rate rad/s (-PI..PI)

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // 0.0-5.0
};

#define SLIT_SCAN_CORRIDOR_CONFIG_FIELDS                                       \
  enabled, slitPosition, slitWidth, speed, perspective, decayHalfLife,         \
      brightness, rotationAngle, rotationSpeed, blendMode, blendIntensity

typedef struct SlitScanCorridorEffect {
  Shader shader;        // Accumulation (ping-pong)
  Shader displayShader; // Rotation pass
  RenderTexture2D pingPong[2];
  int readIdx;
  float rotationAccum;

  // Accumulation shader locations
  int resolutionLoc;
  int sceneTextureLoc;
  int slitPositionLoc;
  int speedDtLoc; // speed * deltaTime (precomputed)
  int perspectiveLoc;
  int slitWidthLoc;
  int decayFactorLoc;
  int brightnessLoc;

  // Display shader locations
  int dispRotationLoc;
} SlitScanCorridorEffect;

typedef struct PostEffect PostEffect;

// Loads shaders, caches uniform locations, allocates ping-pong textures
bool SlitScanCorridorEffectInit(SlitScanCorridorEffect *e,
                                const SlitScanCorridorConfig *cfg, int width,
                                int height);

// Binds scalar uniforms and accumulates rotation state
void SlitScanCorridorEffectSetup(SlitScanCorridorEffect *e,
                                 const SlitScanCorridorConfig *cfg,
                                 float deltaTime);

// Executes ping-pong accumulation pass and display rotation pass
void SlitScanCorridorEffectRender(SlitScanCorridorEffect *e, PostEffect *pe);

// Unloads ping-pong textures, reallocates at new dimensions
void SlitScanCorridorEffectResize(SlitScanCorridorEffect *e, int width,
                                  int height);

// Unloads shaders and ping-pong textures
void SlitScanCorridorEffectUninit(SlitScanCorridorEffect *e);

// Returns default config
SlitScanCorridorConfig SlitScanCorridorConfigDefault(void);

// Registers modulatable params with the modulation engine
void SlitScanCorridorRegisterParams(SlitScanCorridorConfig *cfg);

#endif // SLIT_SCAN_CORRIDOR_H
