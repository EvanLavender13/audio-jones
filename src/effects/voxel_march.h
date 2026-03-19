// Voxel March effect module
// Voxel March: raymarched voxelized sphere shells with FFT-reactive highlights

#ifndef VOXEL_MARCH_H
#define VOXEL_MARCH_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct VoxelMarchConfig {
  bool enabled = false;

  // Geometry
  int marchSteps = 60;         // Raymarching iterations (30-80)
  float stepSize = 0.3f;       // March step multiplier (0.1-0.5)
  float voxelScale = 16.0f;    // Base voxel grid density (4.0-32.0)
  float voxelVariation = 1.0f; // Sin-boundary variation blend (0.0-1.0)
  float cellSize = 4.0f;       // Domain repetition period (1.0-8.0)
  float shellRadius = 2.8f;    // Sphere shell surface radius (0.5-4.0)
  int surfaceCount = 2;        // Shell surfaces (1-2)
  float highlightIntensity =
      0.06f;                   // Boundary seam highlight strength (0.0-0.5)
  float positionTint = 0.5f;   // Position color influence (0.0-1.0)
  float tonemapGain = 0.0005f; // Tonemap exposure (0.0001-0.002)

  // Speed (CPU-accumulated)
  float flySpeed = 1.0f;      // Forward travel speed (0.0-5.0)
  float gridAnimSpeed = 1.0f; // Sin-boundary animation rate (0.0-3.0)

  // Camera
  DualLissajousConfig lissajous = {
      .amplitude = 0.8f, .motionSpeed = 1.0f, .freqX1 = 0.6f, .freqY1 = 0.3f};

  // Audio
  bool colorFreqMap = false; // Map FFT bands to gradient color instead of depth
  float baseFreq = 55.0f;    // Low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f;  // High frequency bound Hz (1000-16000)
  float gain = 2.0f;         // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;        // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f;  // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define VOXEL_MARCH_CONFIG_FIELDS                                              \
  enabled, marchSteps, stepSize, voxelScale, voxelVariation, cellSize,         \
      shellRadius, surfaceCount, highlightIntensity, positionTint,             \
      tonemapGain, flySpeed, gridAnimSpeed, lissajous, colorFreqMap, baseFreq, \
      maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct VoxelMarchEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float flyPhase;
  float gridPhase;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int flyPhaseLoc;
  int gridPhaseLoc;
  int marchStepsLoc;
  int stepSizeLoc;
  int voxelScaleLoc;
  int voxelVariationLoc;
  int cellSizeLoc;
  int shellRadiusLoc;
  int surfaceCountLoc;
  int highlightIntensityLoc;
  int positionTintLoc;
  int tonemapGainLoc;
  int panLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int colorFreqMapLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} VoxelMarchEffect;

// Returns true on success, false if shader fails to load
bool VoxelMarchEffectInit(VoxelMarchEffect *e, const VoxelMarchConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
// Non-const config: DualLissajousUpdate mutates internal phase state
void VoxelMarchEffectSetup(VoxelMarchEffect *e, VoxelMarchConfig *cfg,
                           float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void VoxelMarchEffectUninit(VoxelMarchEffect *e);

// Registers modulatable params with the modulation engine
void VoxelMarchRegisterParams(VoxelMarchConfig *cfg);

#endif // VOXEL_MARCH_H
