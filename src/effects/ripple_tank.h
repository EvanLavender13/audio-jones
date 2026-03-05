// Ripple Tank effect module
// Audio-reactive wave interference from virtual point sources

#ifndef RIPPLE_TANK_H
#define RIPPLE_TANK_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct RippleTankConfig {
  bool enabled = false;
  float waveScale = 50.0f;       // Pattern scale (1-50)
  float falloff = 0.5f;          // Distance attenuation (0-5)
  float visualGain = 1.0f;       // Output intensity (0.5-5)
  int contourCount = 5;          // Number of contour bands/lines (1-20)
  int contourMode = 0;           // Visualization mode (0=off, 1=bands, 2=lines)
  float decayHalfLife = 0.5f;    // Trail persistence seconds (0.1-5)
  int diffusionScale = 4;        // Spatial blur tap spacing (0=off, 1-8)
  int sourceCount = 5;           // Number of sources (1-8)
  float baseRadius = 0.4f;       // Source orbit radius (0.0-0.5)
  DualLissajousConfig lissajous; // Source motion pattern
  bool boundaries = false;       // Enable edge reflections
  float reflectionGain = 0.5f;   // Mirror source attenuation (0.0-1.0)
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend compositor strength (0.0-5.0)
  ColorConfig gradient;
};

#define RIPPLE_TANK_CONFIG_FIELDS                                              \
  enabled, waveScale, falloff, visualGain, contourCount, contourMode,          \
      decayHalfLife, diffusionScale, sourceCount, baseRadius, lissajous,       \
      boundaries, reflectionGain, blendMode, blendIntensity, gradient

typedef struct ColorLUT ColorLUT;

typedef struct RippleTankEffect {
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
  int contourModeLoc;
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
} RippleTankEffect;

// Returns true on success, false if shader fails to load
bool RippleTankEffectInit(RippleTankEffect *e, const RippleTankConfig *cfg,
                          int width, int height);

// Binds all uniforms, computes source positions, updates LUT texture
// Non-const cfg because Lissajous mutates phase each frame.
void RippleTankEffectSetup(RippleTankEffect *e, RippleTankConfig *cfg,
                           float deltaTime, Texture2D waveformTexture,
                           int waveformWriteIndex);

// Renders ripple tank into ping-pong trail buffer with decay blending
void RippleTankEffectRender(RippleTankEffect *e, const RippleTankConfig *cfg,
                            float deltaTime, int screenWidth, int screenHeight);

// Reallocates ping-pong render textures on resolution change
void RippleTankEffectResize(RippleTankEffect *e, int width, int height);

// Unloads shader and frees LUT
void RippleTankEffectUninit(RippleTankEffect *e);

// Returns default config
RippleTankConfig RippleTankConfigDefault(void);

// Registers modulatable params with the modulation engine
void RippleTankRegisterParams(RippleTankConfig *cfg);

#endif // RIPPLE_TANK_H
