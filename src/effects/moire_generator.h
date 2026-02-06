// Moire generator effect module
// Overlays rotatable line gratings to produce interference moire patterns

#ifndef MOIRE_GENERATOR_H
#define MOIRE_GENERATOR_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct MoireLayerConfig {
  float frequency = 20.0f;    // Grating line density (1.0-100.0)
  float angle = 0.0f;         // Static rotation offset in radians
  float rotationSpeed = 0.0f; // Continuous rotation rate in radians/second
  float warpAmount = 0.0f;    // Sinusoidal UV distortion amplitude (0.0-0.5)
  float scale = 1.0f;         // Zoom level (0.5-4.0)
  float phase = 0.0f;         // Wave phase offset in radians
};

struct MoireGeneratorConfig {
  bool enabled = false;

  // Global
  int patternMode = 0;           // 0=Stripes, 1=Circles, 2=Grid
  int layerCount = 3;            // Active layers (2-4)
  bool sharpMode = false;        // Square-wave vs sinusoidal gratings
  float colorIntensity = 0.0f;   // Blend grayscale <-> LUT color (0.0-1.0)
  float globalBrightness = 1.0f; // Overall output brightness (0.0-2.0)

  // Per-layer with staggered frequency/angle defaults
  MoireLayerConfig layer0;
  MoireLayerConfig layer1 = {.frequency = 22.0f, .angle = 0.0873f};
  MoireLayerConfig layer2 = {.frequency = 24.0f, .angle = 0.1745f};
  MoireLayerConfig layer3 = {.frequency = 26.0f, .angle = 0.2618f};

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct ColorLUT ColorLUT;

typedef struct MoireGeneratorEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float layerAngles[4]; // Per-layer rotation accumulators
  float time;           // Global time accumulator for warp animation

  // Uniform locations -- global
  int resolutionLoc;
  int patternModeLoc;
  int layerCountLoc;
  int sharpModeLoc;
  int colorIntensityLoc;
  int globalBrightnessLoc;
  int timeLoc;
  int gradientLUTLoc;

  // Uniform locations -- per-layer (4 each)
  int frequencyLoc[4];
  int angleLoc[4];
  int warpAmountLoc[4];
  int scaleLoc[4];
  int phaseLoc[4];
} MoireGeneratorEffect;

// Returns true on success, false if shader fails to load
bool MoireGeneratorEffectInit(MoireGeneratorEffect *e);

// Binds all uniforms, advances rotation/time accumulators, updates LUT
void MoireGeneratorEffectSetup(MoireGeneratorEffect *e,
                               const MoireGeneratorConfig *cfg,
                               float deltaTime);

// Unloads shader and frees LUT
void MoireGeneratorEffectUninit(MoireGeneratorEffect *e);

// Returns default config with staggered per-layer values
MoireGeneratorConfig MoireGeneratorConfigDefault(void);

// Registers modulatable params with the modulation engine
void MoireGeneratorRegisterParams(MoireGeneratorConfig *cfg);

#endif // MOIRE_GENERATOR_H
