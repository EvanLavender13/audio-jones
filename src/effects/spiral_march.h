// Spiral March effect module
// Spiral March: raymarched flight through a tumbling SDF lattice with
// ray-space rotation that bends straight rays into helical paths

#ifndef SPIRAL_MARCH_H
#define SPIRAL_MARCH_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct SpiralMarchConfig {
  bool enabled = false;

  // Geometry
  int shapeType = 0;        // 0=Octahedron, 1=Box, 2=Sphere, 3=Torus
  float shapeSize = 0.25f;  // Half-extent of SDF primitive (0.05-0.45)
  float cellPitchZ = 0.20f; // Z-axis spacing between cells (0.05-1.0)

  // Raymarching
  int marchSteps = 80;     // Max raymarch iterations (30-120)
  float stepFactor = 0.9f; // Distance multiplier per step (0.5-1.0)
  float maxDist = 100.0f;  // Far clip distance (20-200)
  float spiralRate =
      0.8f; // Ray-space rotation per unit dt (0-3); the doom-spiral knob
  float tColorDistScale = 0.012f; // Color/FFT index from march distance (0-0.1)
  float tColorIterScale =
      0.005f; // Color/FFT index from iteration count (0-0.05)

  // Animation (CPU-accumulated phases)
  float flySpeed = 0.20f; // Forward translation rate (0-5)
  float cellRotateSpeed =
      1.0f; // Per-cell tumble rate rad/s (0-ROTATION_SPEED_MAX)

  // Camera
  float fov = 0.15f;         // Field of view scalar (0.05-1.0)
  float pitchAngle = 0.53f;  // Camera pitch radians (-PI to +PI)
  float rollAngle = -0.531f; // Camera roll radians; reference's 100.0 wrapped
                             // to [-PI, PI] = 100 - 16*PI ~ -0.531

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

#define SPIRAL_MARCH_CONFIG_FIELDS                                             \
  enabled, shapeType, shapeSize, cellPitchZ, marchSteps, stepFactor, maxDist,  \
      spiralRate, tColorDistScale, tColorIterScale, flySpeed, cellRotateSpeed, \
      fov, pitchAngle, rollAngle, baseFreq, maxFreq, gain, curve, baseBright,  \
      gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct SpiralMarchEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float flyPhase;  // accumulates flySpeed * deltaTime
  float cellPhase; // accumulates cellRotateSpeed * deltaTime
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int gradientLUTLoc;
  int flyPhaseLoc;
  int cellPhaseLoc;
  int shapeTypeLoc;
  int shapeSizeLoc;
  int cellPitchZLoc;
  int marchStepsLoc;
  int stepFactorLoc;
  int maxDistLoc;
  int spiralRateLoc;
  int tColorDistScaleLoc;
  int tColorIterScaleLoc;
  int fovLoc;
  int pitchAngleLoc;
  int rollAngleLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} SpiralMarchEffect;

// Returns true on success, false if shader fails to load
bool SpiralMarchEffectInit(SpiralMarchEffect *e, const SpiralMarchConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
// Non-const config: ColorLUTUpdate mutates the LUT cache
void SpiralMarchEffectSetup(SpiralMarchEffect *e, SpiralMarchConfig *cfg,
                            float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void SpiralMarchEffectUninit(SpiralMarchEffect *e);

// Registers modulatable params with the modulation engine
void SpiralMarchRegisterParams(SpiralMarchConfig *cfg);

#endif // SPIRAL_MARCH_H
