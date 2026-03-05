// Cymatics effect module
// Simulated Chladni-plate standing-wave interference patterns

#ifndef CYMATICS_H
#define CYMATICS_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct CymaticsConfig {
  bool enabled = false;
  float waveScale = 10.0f;       // Pattern scale (1-50)
  float falloff = 1.0f;          // Distance attenuation (0-5)
  float visualGain = 2.0f;       // Output intensity (0.5-5)
  int contourCount = 0;          // Banding (0=smooth, 1-10)
  float decayHalfLife = 0.5f;    // Trail persistence seconds (0.1-5)
  int diffusionScale = 4;        // Spatial blur tap spacing (0=off, 1-8)
  int sourceCount = 5;           // Number of sources (1-8)
  float baseRadius = 0.4f;       // Source orbit radius (0.0-0.5)
  DualLissajousConfig lissajous; // Source motion pattern
  bool boundaries = false;       // Enable edge reflections
  float reflectionGain = 1.0f;   // Mirror source attenuation (0.0-1.0)
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend compositor strength (0.0-5.0)
  ColorConfig gradient;
};

#define CYMATICS_CONFIG_FIELDS                                                 \
  enabled, waveScale, falloff, visualGain, contourCount, decayHalfLife,        \
      diffusionScale, sourceCount, baseRadius, lissajous, boundaries,          \
      reflectionGain, blendMode, blendIntensity, gradient

typedef struct ColorLUT ColorLUT;

typedef struct CymaticsEffect {
  Shader shader;
  ColorLUT *colorLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentWaveformTexture; // Stored in Setup, used in Render

  // Uniform locations
  int resolutionLoc;
  int aspectLoc;
  int waveScaleLoc;
  int falloffLoc;
  int visualGainLoc;
  int contourCountLoc;
  int bufferSizeLoc;
  int writeIndexLoc;
  int valueLoc;
  int sourcesLoc;
  int sourceCountLoc;
  int boundariesLoc;
  int reflectionGainLoc;
  int waveformTextureLoc;
  int diffusionScaleLoc;
  int decayFactorLoc;
  int colorLUTLoc;
} CymaticsEffect;

// Returns true on success, false if shader fails to load
bool CymaticsEffectInit(CymaticsEffect *e, const CymaticsConfig *cfg, int width,
                        int height);

// Binds all uniforms, computes source positions, updates LUT texture
// Non-const cfg because Lissajous mutates phase each frame.
void CymaticsEffectSetup(CymaticsEffect *e, CymaticsConfig *cfg,
                         float deltaTime, Texture2D waveformTexture,
                         int waveformWriteIndex);

// Renders cymatics into ping-pong trail buffer with decay blending
void CymaticsEffectRender(CymaticsEffect *e, const CymaticsConfig *cfg,
                          float deltaTime, int screenWidth, int screenHeight);

// Reallocates ping-pong render textures on resolution change
void CymaticsEffectResize(CymaticsEffect *e, int width, int height);

// Unloads shader and frees LUT
void CymaticsEffectUninit(CymaticsEffect *e);

// Returns default config
CymaticsConfig CymaticsConfigDefault(void);

// Registers modulatable params with the modulation engine
void CymaticsRegisterParams(CymaticsConfig *cfg);

#endif // CYMATICS_H
