// Solid color effect module
// Fills screen with a configurable color (solid/rainbow/gradient) for blending

#ifndef SOLID_COLOR_H
#define SOLID_COLOR_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

typedef struct ColorLUT ColorLUT;

struct SolidColorConfig {
  bool enabled = false;
  ColorConfig gradient = {.mode = COLOR_MODE_SOLID};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define SOLID_COLOR_CONFIG_FIELDS enabled, gradient, blendMode, blendIntensity

typedef struct SolidColorEffect {
  Shader shader;
  ColorLUT *colorLUT;
  int colorLUTLoc;
} SolidColorEffect;

// Returns true on success, false if shader fails to load
bool SolidColorEffectInit(SolidColorEffect *e, const SolidColorConfig *cfg);

// Binds color LUT uniform to shader
void SolidColorEffectSetup(SolidColorEffect *e, const SolidColorConfig *cfg);

// Unloads shader and color LUT
void SolidColorEffectUninit(SolidColorEffect *e);

// Registers modulatable params with the modulation engine
void SolidColorRegisterParams(SolidColorConfig *cfg);

SolidColorEffect *GetSolidColorEffect(PostEffect *pe);

#endif // SOLID_COLOR_H
