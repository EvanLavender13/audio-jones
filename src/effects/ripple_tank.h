// Ripple Tank effect module
// Wave interference from virtual point sources with audio and parametric modes

#ifndef RIPPLE_TANK_H
#define RIPPLE_TANK_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct RippleTankConfig {
  bool enabled = false;
  int sourceCount = 5;           // Number of sources (1-8)
  float baseRadius = 0.4f;       // Source orbit radius (0.0-1.0)
  DualLissajousConfig lissajous; // Source motion pattern
  bool boundaries = false;       // Enable edge reflections
  float reflectionGain = 0.5f;   // Mirror source attenuation (0.0-1.0)
  float visualGain = 1.0f;       // Output intensity (0.5-5.0)
  float decayHalfLife = 0.5f;    // Trail persistence seconds (0.1-5.0)
  int diffusionScale = 4;        // Spatial blur tap spacing (0=off, 1-8)

  // Wave source
  int waveSource = 0;      // 0=audio waveform, 1=parametric sine
  float waveScale = 50.0f; // Audio mode pattern scale (1-50)
  float waveFreq = 30.0f;  // Sine mode spatial frequency (5.0-100.0)
  float waveSpeed = 2.0f;  // Sine mode animation speed (0.0-10.0)

  // Attenuation
  float falloffStrength = 0.5f; // Distance attenuation strength (0.0-5.0)
  int falloffType = 3;          // 0=none, 1=inverse, 2=inv-square, 3=gaussian

  // Visualization
  int visualMode = 0;         // 0=raw, 1=absolute, 2=bands, 3=iso-lines
  int contourCount = 5;       // Band/line count (1-20)
  int colorMode = 0;          // 0=intensity, 1=per-source, 2=chromatic
  float chromaSpread = 0.03f; // RGB wavelength spread (0.0-0.1)

  // Output
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend compositor strength (0.0-5.0)
  ColorConfig gradient;
};

#define RIPPLE_TANK_CONFIG_FIELDS                                              \
  enabled, sourceCount, baseRadius, lissajous, boundaries, reflectionGain,     \
      visualGain, decayHalfLife, diffusionScale, waveSource, waveScale,        \
      waveFreq, waveSpeed, falloffStrength, falloffType, visualMode,           \
      contourCount, colorMode, chromaSpread, blendMode, blendIntensity,        \
      gradient

typedef struct ColorLUT ColorLUT;

typedef struct RippleTankEffect {
  Shader shader;
  ColorLUT *colorLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  float time;                       // Accumulator for sine mode animation
  Texture2D currentWaveformTexture; // Stored in Setup, used in Render

  // Uniform locations
  int resolutionLoc;
  int aspectLoc;
  int waveScaleLoc;
  int falloffStrengthLoc;
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
  int timeLoc;
  int waveSourceLoc;
  int waveFreqLoc;
  int falloffTypeLoc;
  int visualModeLoc;
  int colorModeLoc;
  int chromaSpreadLoc;
  int phasesLoc;
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
