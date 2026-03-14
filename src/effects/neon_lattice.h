// Neon Lattice effect module
// Raymarched torus columns on a repeating 3D grid with orbital camera and glow

#ifndef NEON_LATTICE_H
#define NEON_LATTICE_H

#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct NeonLatticeConfig {
  bool enabled = false;

  // Grid
  int axisCount = 3;         // Orthogonal torus column axes (1-3)
  float spacing = 7.0f;      // Grid cell size (2.0-20.0)
  float lightSpacing = 2.0f; // Light repetition multiplier (0.5-5.0)
  float attenuation = 22.0f; // Glow tightness (5.0-60.0)
  float glowExponent = 1.3f; // Glow curve shape (0.5-3.0)

  // Speed (radians/second — accumulated on CPU)
  float cameraSpeed = 1.4f;  // Camera speed (0.0-5.0)
  float columnsSpeed = 2.8f; // Column scroll speed (0.0-15.0)
  float lightsSpeed = 21.0f; // Light streak speed (0.0-60.0)

  // Quality
  int iterations = 50;   // Raymarch step count (20-80)
  float maxDist = 80.0f; // Ray termination distance (20.0-120.0)

  // Torus shape
  float torusRadius = 0.6f; // Torus major radius (0.2-1.5)
  float torusTube = 0.06f;  // Torus tube thickness (0.02-0.2)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define NEON_LATTICE_CONFIG_FIELDS                                             \
  enabled, axisCount, spacing, lightSpacing, attenuation, glowExponent,        \
      cameraSpeed, columnsSpeed, lightsSpeed, iterations, maxDist,             \
      torusRadius, torusTube, gradient, blendMode, blendIntensity

typedef struct ColorLUT ColorLUT;

typedef struct NeonLatticeEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float cameraPhase;  // CPU-accumulated camera time
  float columnsPhase; // CPU-accumulated columns time
  float lightsPhase;  // CPU-accumulated lights time

  int resolutionLoc;
  int spacingLoc;
  int lightSpacingLoc;
  int attenuationLoc;
  int glowExponentLoc;
  int cameraTimeLoc;
  int columnsTimeLoc;
  int lightsTimeLoc;
  int iterationsLoc;
  int maxDistLoc;
  int torusRadiusLoc;
  int torusTubeLoc;
  int gradientLUTLoc;
  int axisCountLoc;
} NeonLatticeEffect;

// Returns true on success, false if shader fails to load
bool NeonLatticeEffectInit(NeonLatticeEffect *e, const NeonLatticeConfig *cfg);

// Accumulates phase, binds all uniforms, updates LUT texture
void NeonLatticeEffectSetup(NeonLatticeEffect *e, const NeonLatticeConfig *cfg,
                            float deltaTime);

// Unloads shader and frees LUT
void NeonLatticeEffectUninit(NeonLatticeEffect *e);

// Returns default config
NeonLatticeConfig NeonLatticeConfigDefault(void);

// Registers modulatable params with the modulation engine
void NeonLatticeRegisterParams(NeonLatticeConfig *cfg);

#endif // NEON_LATTICE_H
