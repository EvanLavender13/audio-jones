// Lichen effect module
// Three-species cyclic reaction-diffusion with coordinate warp and
// gradient-colored output

#ifndef LICHEN_EFFECT_H
#define LICHEN_EFFECT_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

typedef struct ColorLUT ColorLUT;

typedef struct LichenConfig {
  bool enabled = false;

  // Reaction-diffusion
  float feedRate = 0.019f;         // Substrate replenishment (0.005-0.08)
  float killRateBase = 0.084f;     // Base decay rate (0.01-0.12)
  float couplingStrength = 0.04f;  // Cyclic coupling strength (0.0-0.2)
  float predatorAdvantage = 1.07f; // >1 favors prey persistence (0.8-1.5)

  // Coordinate warp
  float warpIntensity = 0.1f; // Boundary stirring amplitude (0.0-0.5)
  float warpSpeed = 2.5f;     // Warp animation rate (0.0-5.0)

  // Diffusion
  float activatorRadius =
      2.5f; // Activator (.x) sample radius in pixels (0.5-5.0)
  float inhibitorRadius =
      1.2f; // Inhibitor (.y) sample radius in pixels (0.5-3.0)

  // Reaction
  int simSteps = 3;          // Full diffusion+reaction passes per frame (1-8)
  int reactionSteps = 25;    // Reaction iterations per frame (5-50)
  float reactionRate = 0.4f; // Time step per iteration (0.1-0.8)

  // Output level
  float brightness = 2.0f; // LUT-color amplifier (0.5-4.0)
  float hueDrift = 0.015f; // Hue random-walk step per growth event (0.0-0.1)

  // Audio (FFT)
  float baseFreq = 55.0f;   // FFT low bound Hz (27.5-440)
  float maxFreq = 14000.0f; // FFT high bound Hz (1000-16000)
  float gain = 2.0f;        // FFT gain (0.1-10)
  float curve = 1.5f;       // FFT contrast curve (0.1-3)
  float baseBright = 0.15f; // Brightness floor (0-1)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
} LichenConfig;

#define LICHEN_CONFIG_FIELDS                                                   \
  enabled, feedRate, killRateBase, couplingStrength, predatorAdvantage,        \
      warpIntensity, warpSpeed, activatorRadius, inhibitorRadius, simSteps,    \
      reactionSteps, reactionRate, brightness, hueDrift, baseFreq, maxFreq,    \
      gain, curve, baseBright, gradient, blendMode, blendIntensity

typedef struct LichenEffect {
  Shader stateShader; // Reaction-diffusion + warp + diffusion shader
  Shader shader;      // Color output shader (sampled by compositor)
  ColorLUT *gradientLUT;
  RenderTexture2D statePingPong0[2]; // RGBA32F: species 0 (rg) + species 1 (ba)
  RenderTexture2D statePingPong1[2]; // RGBA32F: species 2 (rg)
  RenderTexture2D colorRT;           // RGBA32F: per-frame color output
  int readIdx0;
  int readIdx1;

  float time;      // CPU absolute time accumulator (used for hue drift hash)
  float warpPhase; // CPU-accumulated warp phase = sum(warpSpeed * deltaTime)
  Texture2D fftTexture;

  // State shader uniform locations
  int stateResolutionLoc;
  int stateTimeLoc;
  int statePassIndexLoc;
  int stateFeedRateLoc;
  int stateKillRateBaseLoc;
  int stateCouplingStrengthLoc;
  int statePredatorAdvantageLoc;
  int stateWarpIntensityLoc;
  int stateWarpPhaseLoc;
  int stateActivatorRadiusLoc;
  int stateInhibitorRadiusLoc;
  int stateReactionStepsLoc;
  int stateReactionRateLoc;
  int stateHueDriftLoc;
  int stateTex1Loc; // Other state texture (species 2)

  // Color shader uniform locations
  int colorResolutionLoc;
  int colorBrightnessLoc;
  int colorStateTex0Loc;
  int colorStateTex1Loc;
  int colorGradientLUTLoc;
  int colorFftTextureLoc;
  int colorSampleRateLoc;
  int colorBaseFreqLoc;
  int colorMaxFreqLoc;
  int colorGainLoc;
  int colorCurveLoc;
  int colorBaseBrightLoc;
} LichenEffect;

// Loads shaders, caches uniform locations, allocates ping-pong render textures
bool LichenEffectInit(LichenEffect *e, const LichenConfig *cfg, int width,
                      int height);

// Binds uniforms and accumulates warp time
void LichenEffectSetup(LichenEffect *e, const LichenConfig *cfg,
                       float deltaTime, const Texture2D &fftTexture);

// Renders state update and color output passes
void LichenEffectRender(LichenEffect *e, const LichenConfig *cfg,
                        int screenWidth, int screenHeight);

// Reallocates ping-pong textures at new dimensions
void LichenEffectResize(LichenEffect *e, int width, int height);

// Clears state buffers and re-seeds with noise
void LichenEffectReset(LichenEffect *e, int width, int height);

// Unloads shaders, frees LUT and render textures
void LichenEffectUninit(LichenEffect *e);

// Registers modulatable params with the modulation engine
void LichenRegisterParams(LichenConfig *cfg);

LichenEffect *GetLichenEffect(PostEffect *pe);

#endif // LICHEN_EFFECT_H
