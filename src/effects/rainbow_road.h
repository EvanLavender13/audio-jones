// Rainbow Road effect module
// Receding frequency bars with sway, curvature, and glow

#ifndef RAINBOW_ROAD_H
#define RAINBOW_ROAD_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct RainbowRoadConfig {
  bool enabled = false;

  // Geometry
  int layers = 32;          // Number of bars / frequency bands (4-64)
  int direction = 0;        // 0 = recede upward, 1 = recede downward
  float width = 4.0f;       // Bar half-width in world units (0.5-8.0)
  float sway = 1.0f;        // Lateral offset amplitude (0.0-3.0)
  float curvature = 0.4f;   // Per-bar tilt (0.0-1.0)
  float phaseSpread = 0.8f; // Sway/curvature phase multiplier (0.1-3.0)

  // Motion
  float speed = 1.0f; // Scroll speed, positive = toward viewer (-3.0-3.0)

  // Glow
  float glowIntensity = 1.0f; // Glow brightness (0.1-5.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest mapped frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest mapped frequency in Hz (1000-16000)
  float gain = 2.0f;        // FFT energy multiplier (0.1-10.0)
  float curve = 1.5f;       // Energy response curve (0.1-3.0)
  float baseBright =
      0.15f; // Min brightness, 0 = invisible without energy (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define RAINBOW_ROAD_CONFIG_FIELDS                                             \
  enabled, layers, direction, width, sway, curvature, phaseSpread, speed,      \
      glowIntensity, baseFreq, maxFreq, gain, curve, baseBright, gradient,     \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct RainbowRoadEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;   // Animation accumulator for sway
  float scroll; // Speed-accumulated scroll position
  int resolutionLoc;
  int timeLoc;
  int scrollLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int layersLoc;
  int directionLoc;
  int widthLoc;
  int swayLoc;
  int curvatureLoc;
  int phaseSpreadLoc;
  int glowIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} RainbowRoadEffect;

// Returns true on success, false if shader fails to load
bool RainbowRoadEffectInit(RainbowRoadEffect *e, const RainbowRoadConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void RainbowRoadEffectSetup(RainbowRoadEffect *e, const RainbowRoadConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void RainbowRoadEffectUninit(RainbowRoadEffect *e);

// Registers modulatable params with the modulation engine
void RainbowRoadRegisterParams(RainbowRoadConfig *cfg);

RainbowRoadEffect *GetRainbowRoadEffect(PostEffect *pe);

#endif // RAINBOW_ROAD_H
