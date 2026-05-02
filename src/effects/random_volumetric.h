#ifndef RANDOM_VOLUMETRIC_EFFECT_H
#define RANDOM_VOLUMETRIC_EFFECT_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

typedef struct ColorLUT ColorLUT;

struct RandomVolumetricConfig {
  bool enabled = false;

  // Pattern
  float seed = 0.0f;
  float cycleSpeed = 0.4f;
  float iterMin = 4.0f;
  float iterMax = 40.0f;
  float zoom = 4.0f;
  float zoomPulse = 1.5f;

  // Color
  float paletteRandomness = 1.0f;

  // Audio
  float baseFreq = 55.0f;
  float maxFreq = 14000.0f;
  float gain = 2.0f;
  float curve = 1.5f;
  float baseBright = 0.15f;

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define RANDOM_VOLUMETRIC_CONFIG_FIELDS                                        \
  enabled, seed, cycleSpeed, iterMin, iterMax, zoom, zoomPulse,                \
      paletteRandomness, baseFreq, maxFreq, gain, curve, baseBright, gradient, \
      blendMode, blendIntensity

typedef struct RandomVolumetricEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  float time;
  float cameraPhase;
  float cyclePhase;

  int resolutionLoc;
  int timeLoc;
  int cameraPhaseLoc;
  int seedLoc;
  int cyclePhaseLoc;
  int iterMinLoc;
  int iterMaxLoc;
  int zoomLoc;
  int zoomPulseLoc;
  int paletteRandomnessLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} RandomVolumetricEffect;

bool RandomVolumetricEffectInit(RandomVolumetricEffect *e,
                                const RandomVolumetricConfig *cfg);
void RandomVolumetricEffectSetup(RandomVolumetricEffect *e,
                                 const RandomVolumetricConfig *cfg,
                                 float deltaTime, const Texture2D &fftTexture);
void RandomVolumetricEffectUninit(RandomVolumetricEffect *e);
void RandomVolumetricRegisterParams(RandomVolumetricConfig *cfg);

RandomVolumetricEffect *GetRandomVolumetricEffect(PostEffect *pe);

#endif
