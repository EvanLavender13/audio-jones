// Geode effect module
// Geode: raymarched cluster of cubes carved by a gyroid/noise cut field

#ifndef GEODE_H
#define GEODE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct GeodeConfig {
  bool enabled = false;

  // Cluster
  float clusterRadius = 20.0f; // Spherical bound (5.0-60.0)
  float cubeSize =
      0.632f; // Per-cube SDF half-extent (0.3-0.7); default = sqrt(0.40)
  float colorRate =
      0.1f; // Gradient cycle rate along radial distance (0.02-0.5)

  // Cut Field
  int cutMode = 0;                // 0=GYROID, 1=NOISE
  float cutScale = 0.05f;         // Cut-field wavelength (0.02-0.25); 0.05 =
                                  // reference's 1/20 for gyroid
  float cutThresholdBase = 0.0f;  // Static threshold offset (-2.0-2.0)
  float cutThresholdPulse = 0.0f; // Threshold oscillation amplitude (0.0-2.0)
  float cutPulseSpeed = 0.5f; // Threshold oscillation rate rad/sec (0.0-4.0)
  float fieldDriftX = 0.0f;   // Cut-field X drift rate units/sec (-2.0-2.0)
  float fieldDriftY = 0.0f;   // Cut-field Y drift rate units/sec (-2.0-2.0)
  float fieldDriftZ = 0.0f;   // Cut-field Z drift rate units/sec (-2.0-2.0)

  // Camera
  float orbitSpeed =
      0.06f; // Yaw orbit rate rad/sec (-1.0-1.0); reference's 2*pi*0.01
  float orbitPitch = 2.094395f; // Fixed pitch radians (-PI to PI); default
                                // 2*PI/3 from reference
  float cameraDistance = 50.0f; // Camera Z distance pre-rotation (30.0-80.0)

  // Lighting
  float ambient = 0.3f;         // Baseline lighting (0.0-0.5)
  float specularPower = 250.0f; // Specular exponent (10.0-1000.0)
  float fogDistance = 80.0f; // Distance at which fog reaches ~1/e (10.0-100.0)

  // Audio
  float baseFreq = 55.0f;   // FFT low Hz (27.5-440)
  float maxFreq = 14000.0f; // FFT high Hz (1000-16000)
  float gain = 2.0f;        // FFT gain (0.1-10)
  float curve = 1.5f;       // FFT curve (0.1-3)
  float baseBright = 0.15f; // FFT min floor (0-1)

  // Output
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define GEODE_CONFIG_FIELDS                                                    \
  enabled, clusterRadius, cubeSize, colorRate, cutMode, cutScale,              \
      cutThresholdBase, cutThresholdPulse, cutPulseSpeed, fieldDriftX,         \
      fieldDriftY, fieldDriftZ, orbitSpeed, orbitPitch, cameraDistance,        \
      ambient, specularPower, fogDistance, baseFreq, maxFreq, gain, curve,     \
      baseBright, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct GeodeEffect {
  Shader shader;
  ColorLUT *gradientLUT;

  // CPU-accumulated phases (per conventions: shader never sees raw time)
  float cutPulsePhase;
  float orbitPhase;
  float fieldOffsetX;
  float fieldOffsetY;
  float fieldOffsetZ;
  int frame;

  // Uniform locations
  int resolutionLoc;
  int fftTextureLoc;
  int gradientLUTLoc;
  int sampleRateLoc;
  int frameLoc;

  int cutModeLoc;
  int cutScaleLoc;
  int cutThresholdBaseLoc;
  int cutThresholdPulseLoc;
  int cutPulsePhaseLoc;
  int fieldOffsetLoc;
  int clusterRadiusLoc;
  int cubeSizeLoc;
  int colorRateLoc;

  int orbitPhaseLoc;
  int orbitPitchLoc;
  int cameraDistanceLoc;

  int ambientLoc;
  int specularPowerLoc;
  int fogDistanceLoc;

  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} GeodeEffect;

// Returns true on success, false if shader fails to load
bool GeodeEffectInit(GeodeEffect *e, const GeodeConfig *cfg);

// Binds all uniforms including fftTexture, updates LUT texture
void GeodeEffectSetup(GeodeEffect *e, const GeodeConfig *cfg, float deltaTime,
                      const Texture2D &fftTexture);

// Unloads shader and frees LUT
void GeodeEffectUninit(GeodeEffect *e);

// Registers modulatable params with the modulation engine
void GeodeRegisterParams(GeodeConfig *cfg);

#endif // GEODE_H
