// Scrawl effect module
// IFS fractal fold with thick marker strokes and gradient LUT coloring

#ifndef SCRAWL_H
#define SCRAWL_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct ScrawlConfig {
  bool enabled = false;
  int mode = 0; // Fold formula selector (0-6)

  // Geometry
  int iterations = 6;      // Fold depth / curve density (2-12)
  float foldOffset = 0.5f; // Fold attractor shift (0.0-1.5)
  float tileScale = 0.5f;  // Tile repetition density (0.2-2.0)
  float zoom = 0.3f;       // Base view scale (0.1-1.0)
  float warpFreq = 20.0f;  // Sinusoidal wobble frequency (5.0-40.0)
  float warpAmp = 0.3f;    // Sinusoidal wobble amplitude (0.0-0.5)

  // Rendering
  float thickness = 0.015f;   // Stroke width (0.005-0.05)
  float glowIntensity = 1.5f; // Stroke brightness (0.5-5.0)

  // Animation
  float scrollSpeed = 0.3f;    // Horizontal drift speed (-1.0..1.0)
  float evolveSpeed = 0.2f;    // Fold parameter evolution speed (-1.0..1.0)
  float rotationSpeed = 0.0f;  // Global rotation rate rad/s
  float warpPhaseSpeed = 0.0f; // Warp wobble phase drift rate (-2.0..2.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define SCRAWL_CONFIG_FIELDS                                                   \
  enabled, mode, iterations, foldOffset, tileScale, zoom, warpFreq, warpAmp,   \
      thickness, glowIntensity, scrollSpeed, evolveSpeed, rotationSpeed,       \
      warpPhaseSpeed, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct ScrawlEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float scrollAccum;    // CPU-accumulated scroll offset
  float evolveAccum;    // CPU-accumulated evolve phase
  float rotationAccum;  // CPU-accumulated rotation angle
  float warpPhaseAccum; // CPU-accumulated warp wobble phase
  int resolutionLoc;
  int iterationsLoc, foldOffsetLoc, tileScaleLoc, zoomLoc;
  int warpFreqLoc, warpAmpLoc;
  int thicknessLoc, glowIntensityLoc;
  int scrollAccumLoc, evolveAccumLoc, rotationAccumLoc, warpPhaseAccumLoc;
  int gradientLUTLoc;
  int modeLoc;
} ScrawlEffect;

// Returns true on success, false if shader fails to load
bool ScrawlEffectInit(ScrawlEffect *e, const ScrawlConfig *cfg);

// Binds all uniforms and updates LUT texture
void ScrawlEffectSetup(ScrawlEffect *e, const ScrawlConfig *cfg,
                       float deltaTime);

// Unloads shader and frees LUT
void ScrawlEffectUninit(ScrawlEffect *e);

// Registers modulatable params with the modulation engine
void ScrawlRegisterParams(ScrawlConfig *cfg);

ScrawlEffect *GetScrawlEffect(PostEffect *pe);

#endif // SCRAWL_H
