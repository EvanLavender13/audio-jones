// Plaid effect module
// Tartan fabric pattern with twill weave texture driven by FFT semitone energy

#ifndef PLAID_H
#define PLAID_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PlaidConfig {
  bool enabled = false;

  // Fabric
  float scale = 2.0f; // Tiles per screen (0.5-8.0)
  int bandCount = 5;  // Unique bands per sett half, mirrored to 2N (3-8)
  float accentWidth =
      0.15f; // Thin accent stripe width relative to wide bands (0.05-0.5)
  float threadDetail = 128.0f; // Twill texture fineness (16.0-512.0)
  int twillRepeat = 4;         // Twill over/under repeat count (2-8)

  // Animation
  float morphSpeed = 0.3f;  // Band width oscillation speed (0.0-2.0)
  float morphAmount = 0.3f; // Strength of width morphing (0.0-1.0)

  // Glow
  float glowIntensity = 1.0f; // Overall brightness multiplier (0.0-2.0)

  // FFT
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;       // FFT response curve (0.1-3.0)
  float baseBright = 0.3f;  // Minimum band brightness without audio (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define PLAID_CONFIG_FIELDS                                                    \
  enabled, scale, bandCount, accentWidth, threadDetail, twillRepeat,           \
      morphSpeed, morphAmount, glowIntensity, baseFreq, maxFreq, gain, curve,  \
      baseBright, blendIntensity, gradient, blendMode

typedef struct ColorLUT ColorLUT;

typedef struct PlaidEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time; // morphSpeed accumulator
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int scaleLoc;
  int bandCountLoc;
  int accentWidthLoc;
  int threadDetailLoc;
  int twillRepeatLoc;
  int morphAmountLoc;
  int timeLoc;
  int glowIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} PlaidEffect;

// Returns true on success, false if shader fails to load
bool PlaidEffectInit(PlaidEffect *e, const PlaidConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void PlaidEffectSetup(PlaidEffect *e, const PlaidConfig *cfg, float deltaTime,
                      Texture2D fftTexture);

// Unloads shader and frees LUT
void PlaidEffectUninit(PlaidEffect *e);

// Returns default config
PlaidConfig PlaidConfigDefault(void);

// Registers modulatable params with the modulation engine
void PlaidRegisterParams(PlaidConfig *cfg);

#endif // PLAID_H
