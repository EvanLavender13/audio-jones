// Frame Recursion effect module
// Raymarched kaleidoscopic plane-fold IFS wireframe polyhedron tunnel with
// fract(log) self-similar zoom

#ifndef FRAME_RECURSION_H
#define FRAME_RECURSION_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct PostEffect;

struct FrameRecursionConfig {
  bool enabled = false;

  // Shape
  int shape = 2; // 0=Octahedron, 1=Dodecahedron, 2=Icosahedron

  // Rotation (per-axis CPU-accumulated speeds, rad/s)
  float rotateSpeedX = 0.0f;
  float rotateSpeedY = 0.5f;
  float rotateSpeedZ = 0.0f;

  // Zoom
  float zoomSpeed =
      0.5f; // fract(log) tunnel zoom rate (signed; negative reverses)

  // Geometry
  float edgeRadius = 0.01f;
  float glowIntensity = 1.0f;
  int marchSteps = 99;
  int colorMode = 0; // 0=Banded, 1=Layered, 2=Depth

  // Audio (FFT)
  float baseFreq = 55.0f;
  float maxFreq = 14000.0f;
  float gain = 2.0f;
  float curve = 1.5f;
  float baseBright = 0.15f;

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define FRAME_RECURSION_CONFIG_FIELDS                                          \
  enabled, shape, rotateSpeedX, rotateSpeedY, rotateSpeedZ, zoomSpeed,         \
      edgeRadius, glowIntensity, marchSteps, colorMode, baseFreq, maxFreq,     \
      gain, curve, baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct FrameRecursionEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated phases (rad)
  float rotateAngleX;
  float rotateAngleY;
  float rotateAngleZ;
  float zoomPhase;

  // Uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int rotateAngleLoc; // vec3
  int zoomPhaseLoc;
  int shapeLoc;
  int edgeRadiusLoc;
  int glowIntensityLoc;
  int marchStepsLoc;
  int colorModeLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} FrameRecursionEffect;

// Returns true on success, false if shader fails to load
bool FrameRecursionEffectInit(FrameRecursionEffect *e,
                              const FrameRecursionConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
// Non-const config: ColorLUTUpdate mutates the LUT cache
void FrameRecursionEffectSetup(FrameRecursionEffect *e,
                               FrameRecursionConfig *cfg, float deltaTime,
                               const Texture2D &fftTexture);

// Unloads shader and frees LUT
void FrameRecursionEffectUninit(FrameRecursionEffect *e);

// Registers modulatable params with the modulation engine
void FrameRecursionRegisterParams(FrameRecursionConfig *cfg);

FrameRecursionEffect *GetFrameRecursionEffect(PostEffect *pe);

#endif // FRAME_RECURSION_H
