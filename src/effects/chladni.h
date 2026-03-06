// Chladni effect module
// FFT-driven resonant plate eigenmode visualization

#ifndef CHLADNI_H
#define CHLADNI_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ChladniConfig {
  bool enabled = false;

  // Wave
  float plateSize = 1.0f; // Virtual plate scale (0.5-3.0)
  float coherence =
      0.5f; // Sharpens FFT response toward dominant modes (0.0-1.0)
  float visualGain = 1.5f; // Output intensity multiplier (0.5-5.0)
  float nodalEmphasis =
      0.0f;           // 0=signed field, 1=abs (nodal line emphasis) (0.0-1.0)
  int plateShape = 0; // 0=rectangular, 1=circular (0-1)
  bool fullscreen = true; // true=fill screen, false=mask to plate boundary

  // Audio
  float baseFreq = 55.0f;   // Lowest resonant frequency (27.5-440)
  float maxFreq = 14000.0f; // Highest resonant frequency (1000-16000)
  float gain = 2.0f;        // FFT energy sensitivity (0.1-10.0)
  float curve = 1.5f;       // Contrast exponent on FFT energy (0.1-3.0)

  // Trail
  float decayHalfLife = 0.3f; // Trail persistence seconds (0.05-5.0)
  int diffusionScale = 2;     // Spatial blur tap spacing (0-8)

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend compositor strength (0.0-5.0)
  ColorConfig gradient;
};

#define CHLADNI_CONFIG_FIELDS                                                  \
  enabled, plateSize, coherence, visualGain, nodalEmphasis, plateShape,        \
      fullscreen, baseFreq, maxFreq, gain, curve, decayHalfLife,               \
      diffusionScale, blendMode, blendIntensity, gradient

typedef struct ColorLUT ColorLUT;

typedef struct ChladniEffect {
  Shader shader;
  ColorLUT *colorLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFftTexture;

  // Uniform locations
  int resolutionLoc;
  int plateSizeLoc;
  int coherenceLoc;
  int visualGainLoc;
  int nodalEmphasisLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int plateShapeLoc;
  int fullscreenLoc;
  int diffusionScaleLoc;
  int decayFactorLoc;
  int gradientLUTLoc;
} ChladniEffect;

// Returns true on success, false if shader fails to load
bool ChladniEffectInit(ChladniEffect *e, const ChladniConfig *cfg, int width,
                       int height);

// Binds all uniforms, updates LUT texture
void ChladniEffectSetup(ChladniEffect *e, const ChladniConfig *cfg,
                        float deltaTime, Texture2D fftTexture);

// Renders Chladni pattern into ping-pong trail buffer with decay blending
void ChladniEffectRender(ChladniEffect *e, const ChladniConfig *cfg,
                         float deltaTime, int screenWidth, int screenHeight);

// Reallocates ping-pong render textures on resolution change
void ChladniEffectResize(ChladniEffect *e, int width, int height);

// Unloads shader and frees LUT
void ChladniEffectUninit(ChladniEffect *e);

// Returns default config
ChladniConfig ChladniConfigDefault(void);

// Registers modulatable params with the modulation engine
void ChladniRegisterParams(ChladniConfig *cfg);

#endif // CHLADNI_H
