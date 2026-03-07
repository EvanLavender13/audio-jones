// Curl advection effect module
// GPU curl-noise fluid advection with divergence feedback and gradient-colored
// trails

#ifndef CURL_ADVECTION_H
#define CURL_ADVECTION_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

typedef struct ColorLUT ColorLUT;

typedef struct CurlAdvectionConfig {
  bool enabled = false;
  int steps = 40;                   // Advection iterations (10-80)
  float advectionCurl = 0.2f;       // How much paths spiral (0.0-1.0)
  float curlScale = -2.0f;          // Vortex rotation strength (-4.0-4.0)
  float laplacianScale = 0.05f;     // Diffusion/smoothing (0.0-0.2)
  float pressureScale = -2.0f;      // Compression waves (-4.0-4.0)
  float divergenceScale = -0.4f;    // Source/sink strength (-1.0-1.0)
  float divergenceUpdate = -0.03f;  // Divergence feedback rate (-0.1-0.1)
  float divergenceSmoothing = 0.3f; // Divergence smoothing (0.0-0.5)
  float selfAmp = 1.0f;             // Self-amplification (0.5-2.0)
  float updateSmoothing = 0.4f;     // Temporal stability (0.25-0.9)
  float injectionIntensity = 0.0f;  // Energy injection (0.0-1.0)
  float injectionThreshold = 0.1f;  // Accum brightness cutoff (0.0-1.0)
  float decayHalfLife = 0.5f;       // Trail decay half-life (0.1-5.0 s)
  int diffusionScale = 0;           // Trail diffusion tap spacing (0-4)
  float boostIntensity = 1.0f;      // Trail boost strength (0.0-5.0)
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  ColorConfig color;
  bool debugOverlay = false;
} CurlAdvectionConfig;

#define CURL_ADVECTION_CONFIG_FIELDS                                           \
  enabled, steps, advectionCurl, curlScale, laplacianScale, pressureScale,     \
      divergenceScale, divergenceUpdate, divergenceSmoothing, selfAmp,         \
      updateSmoothing, injectionIntensity, injectionThreshold, decayHalfLife,  \
      diffusionScale, boostIntensity, blendMode, color, debugOverlay

typedef struct CurlAdvectionEffect {
  Shader stateShader; // State update fragment shader
  Shader shader;      // Color output fragment shader
  ColorLUT *colorLUT;
  RenderTexture2D
      statePingPong[2]; // RGBA32F velocity state (xy=velocity, z=divergence)
  RenderTexture2D pingPong[2]; // RGBA32F visual output with decay
  int stateReadIdx;
  int readIdx;
  Texture2D currentAccumTexture; // Cached per-frame for render pass

  // State shader uniform locations
  int stateResolutionLoc;
  int stateStepsLoc;
  int stateAdvectionCurlLoc;
  int stateCurlScaleLoc;
  int stateLaplacianScaleLoc;
  int statePressureScaleLoc;
  int stateDivergenceScaleLoc;
  int stateDivergenceUpdateLoc;
  int stateDivergenceSmoothingLoc;
  int stateSelfAmpLoc;
  int stateUpdateSmoothingLoc;
  int stateInjectionIntensityLoc;
  int stateInjectionThresholdLoc;
  int stateAccumTextureLoc;

  // Color shader uniform locations
  int colorStateTextureLoc;
  int colorDecayFactorLoc;
  int colorLUTLoc;
  int colorValueLoc;
  int colorResolutionLoc;
  int colorDiffusionScaleLoc;
} CurlAdvectionEffect;

// Loads shaders, caches uniform locations, allocates ping-pong render textures
bool CurlAdvectionEffectInit(CurlAdvectionEffect *e,
                             const CurlAdvectionConfig *cfg, int width,
                             int height);

// Binds uniforms and accumulates state
void CurlAdvectionEffectSetup(CurlAdvectionEffect *e,
                              const CurlAdvectionConfig *cfg, float deltaTime);

// Renders state update and color output passes
void CurlAdvectionEffectRender(CurlAdvectionEffect *e,
                               const CurlAdvectionConfig *cfg, float deltaTime,
                               int screenWidth, int screenHeight);

// Reallocates ping-pong textures at new dimensions
void CurlAdvectionEffectResize(CurlAdvectionEffect *e, int width, int height);

// Clears visual buffers and re-seeds state with noise
void CurlAdvectionEffectReset(CurlAdvectionEffect *e, int width, int height);

// Unloads shaders, frees LUT and render textures
void CurlAdvectionEffectUninit(CurlAdvectionEffect *e);

// Returns default config
CurlAdvectionConfig CurlAdvectionConfigDefault(void);

// Registers modulatable params with the modulation engine
void CurlAdvectionRegisterParams(CurlAdvectionConfig *cfg);

#endif // CURL_ADVECTION_H
