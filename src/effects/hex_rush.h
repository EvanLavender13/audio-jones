// Hex rush effect module
// FFT-driven concentric polygon walls rushing inward with gap patterns,
// perspective distortion, and gradient coloring

#ifndef HEX_RUSH_H
#define HEX_RUSH_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct HexRushConfig {
  bool enabled = false;

  // FFT mapping
  float baseFreq = 55.0f;   // Lowest visible frequency in Hz (27.5-440.0)
  float maxFreq = 14000.0f; // Highest visible frequency in Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplifier (0.1-10.0)
  float curve = 1.0f;       // Contrast exponent on magnitude (0.1-3.0)
  float baseBright = 0.1f;  // Minimum brightness floor (0.0-1.0)
  int freqBins = 48; // Discrete frequency bins for ring FFT lookup (12-120)

  // Geometry
  int sides = 6;               // Number of angular segments (3-12)
  float centerSize = 0.15f;    // Center polygon radius (0.05-0.5)
  float wallThickness = 0.15f; // Radial thickness of wall bands (0.02-0.6)
  float wallSpacing = 0.5f;    // Distance between wall rings (0.2-2.0)

  // Dynamics
  float wallSpeed = 1.5f;  // Base inward rush speed (0.5-10.0)
  float gapChance = 0.35f; // Probability a segment is open per ring (0.1-0.99)
  float rotationSpeed = 0.5f; // Global rotation rate (rad/s, -PI..PI)
  float flipRate = 0.1f;   // Rotation direction reversal frequency Hz (0.0-1.0)
  float pulseSpeed = 0.3f; // Center polygon pulse frequency Hz (0.0-2.0)
  float pulseAmount = 0.02f; // Center polygon pulse intensity (0.0-0.5)
  float patternSeed = 0.0f;  // Seed for wall pattern hash (0.0-100.0)

  // Visual
  float perspective = 0.3f; // Pseudo-3D perspective distortion (0.0-1.0)
  float bgContrast =
      0.3f; // Brightness diff between alternating segments (0.0-1.0)
  float colorSpeed = 0.1f;    // Color cycle speed through gradient (0.0-1.0)
  float wallGlow = 0.5f;      // Soft glow width on wall edges (0.0-2.0)
  float glowIntensity = 1.0f; // Overall brightness multiplier (0.1-3.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define HEX_RUSH_CONFIG_FIELDS                                                 \
  enabled, baseFreq, maxFreq, gain, curve, baseBright, freqBins, sides,        \
      centerSize, wallThickness, wallSpacing, wallSpeed, gapChance,            \
      rotationSpeed, flipRate, pulseSpeed, pulseAmount, patternSeed,           \
      perspective, bgContrast, colorSpeed, wallGlow, glowIntensity, gradient,  \
      blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct HexRushEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAccum;    // CPU-accumulated rotation phase
  float flipAccum;        // CPU-accumulated flip timer
  float rotationDir;      // Current rotation direction (+1 or -1)
  float pulseAccum;       // CPU-accumulated pulse timer
  float wallAccum;        // CPU-accumulated wall depth (wallSpeed * dt)
  float wobbleTime;       // Perspective wobble clock
  float colorAccum;       // CPU-accumulated color cycle phase
  float ringBuffer[1024]; // CPU-side RGBA32F pairs (256 entries x 4 floats)
  Texture2D ringBufferTex;
  int lastFilledRing;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int sidesLoc;
  int centerSizeLoc;
  int wallThicknessLoc;
  int wallSpacingLoc;
  int rotationAccumLoc;
  int pulseAmountLoc;
  int pulseAccumLoc;
  int perspectiveLoc;
  int bgContrastLoc;
  int colorAccumLoc;
  int wallGlowLoc;
  int glowIntensityLoc;
  int wallAccumLoc;
  int wobbleTimeLoc;
  int gradientLUTLoc;
  int ringBufferLoc;
  int freqBinsLoc;
} HexRushEffect;

// Returns true on success, false if shader fails to load
bool HexRushEffectInit(HexRushEffect *e, const HexRushConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void HexRushEffectSetup(HexRushEffect *e, const HexRushConfig *cfg,
                        float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void HexRushEffectUninit(HexRushEffect *e);

// Returns default config
HexRushConfig HexRushConfigDefault(void);

// Registers modulatable params with the modulation engine
void HexRushRegisterParams(HexRushConfig *cfg);

#endif // HEX_RUSH_H
