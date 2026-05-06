// Calligraph effect module
// Two-pass ping-pong feedback ink simulation: lissajous-traced curve seeds a
// mask field that gets advected by procedural curl-noise and rendered with
// edge detection through a gradient LUT

#ifndef CALLIGRAPH_EFFECT_H
#define CALLIGRAPH_EFFECT_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

typedef struct ColorLUT ColorLUT;

typedef struct CalligraphConfig {
  bool enabled = false;

  // Curl noise field
  float morphSpeed = 0.25f; // Noise field morph rate (0.0-2.0)

  // Feedback
  float decayRate = 1.0f;     // Mask fade per second (0.05-5.0)
  float spawnDistort = 0.05f; // Curl distortion of spawn position (0.0-0.2)
  float advectScale = 0.005f; // Curl advection of prior frame (0.0-0.02)

  // Spawn shape (lissajous polyline SDF)
  float lineThickness =
      0.02f;                 // Spawn-line thickness in centered NDC (0.005-0.1)
  int lissajousSamples = 64; // Polyline samples for SDF (16-256)
  DualLissajousConfig lissajous = {
      .amplitude = 0.4f,
      .motionSpeed = 0.0f,
      .freqX1 = 1.0f,
      .freqY1 = 1.0f,
  };

  // Render
  float edgeFeather = 0.02f; // Edge-detection feather (0.005-0.1)

  // FFT (standard)
  float baseFreq = 55.0f;
  float maxFreq = 14000.0f;
  float gain = 2.0f;
  float curve = 1.5f;
  float baseBright = 0.15f;

  // Output (standard generator)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
} CalligraphConfig;

#define CALLIGRAPH_CONFIG_FIELDS                                                  \
  enabled, morphSpeed, decayRate, spawnDistort, advectScale, lineThickness,    \
      lissajousSamples, lissajous, edgeFeather, baseFreq, maxFreq, gain,       \
      curve, baseBright, gradient, blendMode, blendIntensity

typedef struct CalligraphEffect {
  Shader stateShader; // Mask field update shader (Pass A)
  Shader shader;      // Color render shader (Pass Image)
  ColorLUT *gradientLUT;
  RenderTexture2D maskPingPong[2]; // RGBA32F mask field (red channel used)
  RenderTexture2D colorRT;         // RGBA32F final colored output
  int readIdx;

  float morphPhase; // CPU-accumulated noise time
  Texture2D fftTexture;

  // State shader uniform locations
  int stateResolutionLoc;
  int stateMorphPhaseLoc;
  int stateDeltaTimeLoc;
  int stateDecayRateLoc;
  int stateSpawnDistortLoc;
  int stateAdvectScaleLoc;
  int stateLineThicknessLoc;
  int stateLissajousSamplesLoc;
  int stateLissAmplitudeLoc;
  int stateLissPhaseLoc;
  int stateLissFreqX1Loc;
  int stateLissFreqY1Loc;
  int stateLissFreqX2Loc;
  int stateLissFreqY2Loc;
  int stateLissOffsetX2Loc;
  int stateLissOffsetY2Loc;

  // Color shader uniform locations
  int colorResolutionLoc;
  int colorEdgeFeatherLoc;
  int colorFftTextureLoc;
  int colorGradientLUTLoc;
  int colorSampleRateLoc;
  int colorBaseFreqLoc;
  int colorMaxFreqLoc;
  int colorGainLoc;
  int colorCurveLoc;
  int colorBaseBrightLoc;
} CalligraphEffect;

// Loads shaders, caches uniform locations, allocates ping-pong render textures
bool CalligraphEffectInit(CalligraphEffect *e, const CalligraphConfig *cfg, int width,
                       int height);

// Binds uniforms and accumulates morph phase
void CalligraphEffectSetup(CalligraphEffect *e, CalligraphConfig *cfg, float deltaTime,
                        const Texture2D &fftTexture);

// Renders mask state update and colored output passes
void CalligraphEffectRender(CalligraphEffect *e, const CalligraphConfig *cfg,
                         int screenWidth, int screenHeight);

// Reallocates ping-pong textures at new dimensions
void CalligraphEffectResize(CalligraphEffect *e, int width, int height);

// Unloads shaders, frees LUT and render textures
void CalligraphEffectUninit(CalligraphEffect *e);

// Registers modulatable params with the modulation engine
void CalligraphRegisterParams(CalligraphConfig *cfg);

CalligraphEffect *GetCalligraphEffect(PostEffect *pe);

#endif // CALLIGRAPH_EFFECT_H
