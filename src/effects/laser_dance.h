// Laser Dance generator
// Raymarched cosine field with animated laser-like beams

#ifndef LASER_DANCE_H
#define LASER_DANCE_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct LaserDanceConfig {
  bool enabled = false;

  // Geometry
  float speed = 1.0f;      // Animation rate (0.1-5.0)
  float freqRatio = 0.6f;  // Cosine field ratio (0.3-1.5)
  float brightness = 1.0f; // Output brightness (0.5-3.0)

  // Warp
  float warpAmount = 0.0f; // Perturbation strength (0.0-1.5)
  float warpSpeed = 1.0f;  // Warp evolution speed (0.1-3.0)
  float warpFreq = 0.4f;   // Spatial tightness (0.1-2.0)

  // Camera
  float zoom = 1.0f;          // Focal length / FOV (0.5-3.0)
  float rotationSpeed = 0.0f; // Camera rotation rate rad/s (-PI_F to PI_F)
  DualLissajousConfig drift = {
      .amplitude = 16.0f,
      .motionSpeed = 1.0f,
      .freqX1 = 0.07f,
      .freqY1 = 0.05f,
      .freqX2 = 0.0f,
      .freqY2 = 0.0f,
  };

  // Audio
  float baseFreq = 55.0f;   // Lowest FFT frequency (27.5-440.0)
  float maxFreq = 14000.0f; // Highest FFT frequency (1000-16000)
  float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;       // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f; // Min brightness when silent (0.0-1.0)

  // Color
  float colorSpeed = 0.5f; // Gradient cycle speed (0.0-3.0)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define LASER_DANCE_CONFIG_FIELDS                                              \
  enabled, speed, freqRatio, brightness, warpAmount, warpSpeed, warpFreq,      \
      zoom, rotationSpeed, drift, baseFreq, maxFreq, gain, curve, baseBright,  \
      colorSpeed, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct LaserDanceEffect {
  Shader shader;
  int resolutionLoc;
  int timeLoc;
  int freqRatioLoc;
  int brightnessLoc;
  int warpAmountLoc;
  int warpTimeLoc;
  int warpFreqLoc;
  int colorPhaseLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int cameraDriftLoc;
  int cameraAngleLoc;
  int zoomLoc;
  float time;
  float warpTime;
  float colorPhase;
  float cameraAngle;
  ColorLUT *gradientLUT;
} LaserDanceEffect;

// Returns true on success, false if shader fails to load
bool LaserDanceEffectInit(LaserDanceEffect *e, const LaserDanceConfig *cfg);

// Non-const config: DualLissajousUpdate mutates internal phase state
void LaserDanceEffectSetup(LaserDanceEffect *e, LaserDanceConfig *cfg,
                           float deltaTime, const Texture2D &fftTexture);

// Unloads shader and frees LUT
void LaserDanceEffectUninit(LaserDanceEffect *e);

// Registers modulatable params with the modulation engine
void LaserDanceRegisterParams(LaserDanceConfig *cfg);

#endif // LASER_DANCE_H
