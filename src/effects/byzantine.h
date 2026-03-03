// Byzantine effect module
// Dual-pass cellular automaton with zoom-reseed cycles and gradient LUT display

#ifndef BYZANTINE_H
#define BYZANTINE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ByzantineConfig {
  bool enabled = false;

  // Simulation
  float diffusionWeight = 0.367879f; // Even-frame center weight, 1/e (0.1-0.9)
  float sharpenWeight = 3.0f;        // Odd-frame center weight (1.5-5.0)
  float cycleLength = 360.0f;        // Frames between zoom reseeds (60-600)
  float zoomAmount = 2.0f;           // Zoom factor per reseed (1.2-4.0)
  float centerX = 0.5f;              // Zoom focus X in UV (0.0-1.0)
  float centerY = 0.5f;              // Zoom focus Y in UV (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};

#define BYZANTINE_CONFIG_FIELDS                                                \
  enabled, diffusionWeight, sharpenWeight, cycleLength, zoomAmount, centerX,   \
      centerY, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;
typedef struct PostEffect PostEffect;

typedef struct ByzantineEffect {
  Shader shader;        // Simulation shader (ping-pong)
  Shader displayShader; // Display pass (counter-zoom + LUT)
  RenderTexture2D pingPong[2];
  int readIdx;
  int frameCount;
  int cachedCycleLen; // Cached (int)cycleLength for Render
  ColorLUT *gradientLUT;

  // Sim shader locations
  int simResolutionLoc;
  int simFrameCountLoc;
  int simCycleLengthLoc;
  int simDiffusionWeightLoc;
  int simSharpenWeightLoc;
  int simZoomAmountLoc;
  int simCenterLoc;

  // Display shader locations
  int dispResolutionLoc;
  int dispCycleProgressLoc;
  int dispZoomAmountLoc;
  int dispCenterLoc;
  int dispGradientLUTLoc;
} ByzantineEffect;

// Returns true on success, false if shader fails to load
bool ByzantineEffectInit(ByzantineEffect *e, const ByzantineConfig *cfg,
                         int width, int height);

// Binds all uniforms, advances frame counter, updates LUT texture
void ByzantineEffectSetup(ByzantineEffect *e, const ByzantineConfig *cfg,
                          float deltaTime);

// Runs simulation step and display pass into post-effect chain
void ByzantineEffectRender(ByzantineEffect *e, PostEffect *pe);

// Reallocates ping-pong render textures on resolution change
void ByzantineEffectResize(ByzantineEffect *e, int width, int height);

// Unloads shaders and frees LUT
void ByzantineEffectUninit(ByzantineEffect *e);

// Returns default config
ByzantineConfig ByzantineConfigDefault(void);

// Registers modulatable params with the modulation engine
void ByzantineRegisterParams(ByzantineConfig *cfg);

#endif // BYZANTINE_H
