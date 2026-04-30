// Apollonian Tunnel effect module
// Raymarched Apollonian-gasket fractal carved into a sinusoidal tunnel with
// FFT-driven volumetric glow (based on "Fractal Tunnel Flight" by diatribes)

#ifndef APOLLONIAN_TUNNEL_H
#define APOLLONIAN_TUNNEL_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ApollonianTunnelConfig {
  bool enabled = false;

  // Geometry
  int marchSteps = 96;    // Raymarch iterations per pixel (48-128)
  int fractalIters = 6;   // Apollonian fold-invert iterations (3-10)
  float preScale = 16.0f; // Fractal feature size relative to world (4.0-32.0)
  float verticalOffset =
      -2.0f;                 // Vertical shift through the lattice (-4.0 to 4.0)
  float tunnelRadius = 2.0f; // Carved tunnel cylinder radius (0.5-4.0)

  // Tunnel
  float pathAmplitude = 16.0f; // Lateral weave amplitude (4.0-32.0)
  float pathFreq = 0.07f;      // Lateral weave frequency along Z (0.01-0.2)

  // Animation (CPU-accumulated phases)
  float flySpeed = 1.5f;   // Forward travel speed (0.0-5.0)
  float rollSpeed = 0.15f; // Camera roll rate rad/s (-2.0 to 2.0)
  float rollAmount = 0.3f; // Camera roll intensity (0.0-1.0)

  // Glow
  float glowIntensity =
      1.5e-4f;               // Volumetric glow gain, log scale (1e-5 to 1e-2)
  float fogDensity = 0.025f; // Depth fog thickness (0.0-0.2, higher = thicker)
  float depthCycle = 12.6f;  // Gradient/FFT cycle period along depth (2.0-50.0)

  // Audio (FFT)
  float baseFreq = 55.0f;   // Lowest mapped pitch (27.5-440)
  float maxFreq = 14000.0f; // Highest mapped pitch (1000-16000)
  float gain = 2.0f;        // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;       // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define APOLLONIAN_TUNNEL_CONFIG_FIELDS                                        \
  enabled, marchSteps, fractalIters, preScale, verticalOffset, tunnelRadius,   \
      pathAmplitude, pathFreq, flySpeed, rollSpeed, rollAmount, glowIntensity, \
      fogDensity, depthCycle, baseFreq, maxFreq, gain, curve, baseBright,      \
      gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct ApollonianTunnelEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float flyPhase;  // accumulates flySpeed * deltaTime
  float rollPhase; // accumulates rollSpeed * deltaTime
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int flyPhaseLoc;
  int rollPhaseLoc;
  int marchStepsLoc;
  int fractalItersLoc;
  int preScaleLoc;
  int verticalOffsetLoc;
  int tunnelRadiusLoc;
  int pathAmplitudeLoc;
  int pathFreqLoc;
  int rollAmountLoc;
  int glowIntensityLoc;
  int fogDensityLoc;
  int depthCycleLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} ApollonianTunnelEffect;

// Returns true on success, false if shader fails to load
bool ApollonianTunnelEffectInit(ApollonianTunnelEffect *e,
                                const ApollonianTunnelConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
// Non-const config: ColorLUTUpdate mutates the LUT cache
void ApollonianTunnelEffectSetup(ApollonianTunnelEffect *e,
                                 ApollonianTunnelConfig *cfg, float deltaTime,
                                 const Texture2D &fftTexture);

// Unloads shader and frees LUT
void ApollonianTunnelEffectUninit(ApollonianTunnelEffect *e);

// Registers modulatable params with the modulation engine
void ApollonianTunnelRegisterParams(ApollonianTunnelConfig *cfg);

#endif // APOLLONIAN_TUNNEL_H
