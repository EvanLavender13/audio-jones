// LED Cube effect module
// Rotating 3D lattice of FFT-reactive LED points with tracer-driven highlights

#ifndef LED_CUBE_H
#define LED_CUBE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct LedCubeConfig {
  bool enabled = false;

  // Grid
  int gridSize = 5; // Points per axis, gridSize^3 total LEDs (3-10)
  float glowFalloff =
      0.15f; // Point glow spread - sharp pinprick to soft halo (0.01-1.0)
  float displacement =
      0.0f; // Grid loosening - 0=rigid lattice, 1=organic drift (0.0-1.0)

  // Camera
  float cubeScale = 4.5f; // Apparent cube size in viewport (2.0-10.0)
  float fovDiv = 2.2f;    // Perspective divisor - higher=flatter (1.0-4.0)

  // Speed (radians/second, accumulated on CPU)
  float tracerSpeed = 1.0f; // Edge tracer traversal speed (0.1-4.0)
  float rotSpeedA = 0.25f;  // First-axis rotation rate (-PI to PI)
  float rotSpeedB = 0.15f;  // Second-axis rotation rate (-PI to PI)
  float driftSpeed = 1.0f;  // Displacement drift rate (0.0-5.0)

  // Audio
  float baseFreq = 55.0f;   // FFT low bound Hz (27.5-440)
  float maxFreq = 14000.0f; // FFT high bound Hz (1000-16000)
  float gain = 2.0f;        // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;       // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f; // Minimum LED brightness (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define LED_CUBE_CONFIG_FIELDS                                                 \
  enabled, gridSize, glowFalloff, displacement, cubeScale, fovDiv,             \
      tracerSpeed, rotSpeedA, rotSpeedB, driftSpeed, baseFreq, maxFreq, gain,  \
      curve, baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct LedCubeEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated phases
  float tracerPhase;
  float rotPhaseA;
  float rotPhaseB;
  float driftPhase;

  // Uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int gridSizeLoc;
  int tracerPhaseLoc;
  int rotPhaseALoc;
  int rotPhaseBLoc;
  int driftPhaseLoc;
  int glowFalloffLoc;
  int displacementLoc;
  int cubeScaleLoc;
  int fovDivLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} LedCubeEffect;

// Returns true on success, false if shader fails to load
bool LedCubeEffectInit(LedCubeEffect *e, const LedCubeConfig *cfg);

// Accumulates phase, binds all uniforms, updates LUT texture
void LedCubeEffectSetup(LedCubeEffect *e, const LedCubeConfig *cfg,
                        float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void LedCubeEffectUninit(LedCubeEffect *e);

// Registers modulatable params with the modulation engine
void LedCubeRegisterParams(LedCubeConfig *cfg);

LedCubeEffect *GetLedCubeEffect(PostEffect *pe);

#endif // LED_CUBE_H
