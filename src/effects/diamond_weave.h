// DiamondWeave effect module
// Twisted diamond tile grid with FFT-driven ringing patterns and radial twist

#ifndef DIAMOND_WEAVE_H
#define DIAMOND_WEAVE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct DiamondWeaveConfig {
  bool enabled = false;

  // Geometry
  float cellSize = 80.0f;   // Pixel size of each diamond tile (20-200)
  float baseAngle = 0.777f; // Static rotation of grid (-PI to PI)

  // Animation
  float phaseSpeed = 0.3f;    // Ring evolution rate (-1.0 to 1.0)
  float rotationSpeed = 0.0f; // Uniform rotation rate, rad/s (-PI to PI)
  float twistSpeed = 0.02f;   // Differential swirl rate, rad/s (-PI to PI)
  float driftSpeed = 0.1f;    // Gradient hue drift rate (-0.5 to 0.5)

  // Glow
  float glowIntensity = 1.0f; // Output brightness multiplier (0.0-2.0)

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency Hz (27.5-440)
  float maxFreq = 14000.0f; // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.5f;       // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define DIAMOND_WEAVE_CONFIG_FIELDS                                            \
  enabled, cellSize, baseAngle, phaseSpeed, rotationSpeed, twistSpeed,         \
      driftSpeed, glowIntensity, baseFreq, maxFreq, gain, curve, baseBright,   \
      gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct DiamondWeaveEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  float phaseAccum;
  float rotationAccum;
  float twistAccum;
  float driftAccum;

  // Shader uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int gradientLUTLoc;
  int sampleRateLoc;
  int phaseAccumLoc;
  int rotationAccumLoc;
  int twistAccumLoc;
  int driftAccumLoc;
  int cellSizeLoc;
  int baseAngleLoc;
  int glowIntensityLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} DiamondWeaveEffect;

// Returns true on success, false if shader fails to load
bool DiamondWeaveEffectInit(DiamondWeaveEffect *e,
                            const DiamondWeaveConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void DiamondWeaveEffectSetup(DiamondWeaveEffect *e,
                             const DiamondWeaveConfig *cfg, float deltaTime,
                             const Texture2D &fftTexture);

// Unloads shader and frees LUT
void DiamondWeaveEffectUninit(DiamondWeaveEffect *e);

// Registers modulatable params with the modulation engine
void DiamondWeaveRegisterParams(DiamondWeaveConfig *cfg);

DiamondWeaveEffect *GetDiamondWeaveEffect(PostEffect *pe);

#endif // DIAMOND_WEAVE_H
