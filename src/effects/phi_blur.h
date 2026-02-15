// Phi Blur effect module
// Golden-ratio sampled blur with disc, box, hex, and star kernel shapes

#ifndef PHI_BLUR_H
#define PHI_BLUR_H

#include "raylib.h"
#include <stdbool.h>

// PhiBlur: Golden-ratio distributed blur samples with configurable kernel shape
// Supports disc, box, hexagonal, and star kernel modes
struct PhiBlurConfig {
  bool enabled = false;
  int shape = 0;                // 0=Disc, 1=Box, 2=Hex, 3=Star
  float radius = 5.0f;          // Blur extent in pixels (0.0-50.0)
  float shapeAngle = 0.0f;      // Kernel rotation in radians (0-2pi)
  int starPoints = 5;           // Star point count (3-8)
  float starInnerRadius = 0.4f; // Star valley depth (0.1-0.9)
  int samples = 32;             // Sample count (8-128)
  float gamma = 2.2f;           // Blending gamma (1.0-6.0)
};

#define PHI_BLUR_CONFIG_FIELDS                                                 \
  enabled, shape, radius, shapeAngle, starPoints, starInnerRadius, samples,    \
      gamma

typedef struct PhiBlurEffect {
  Shader shader;
  int resolutionLoc;
  int shapeLoc;
  int radiusLoc;
  int shapeAngleLoc;
  int starPointsLoc;
  int starInnerRadiusLoc;
  int samplesLoc;
  int gammaLoc;
} PhiBlurEffect;

// Returns true on success, false if shader fails to load
bool PhiBlurEffectInit(PhiBlurEffect *e);

// Sets all uniforms
void PhiBlurEffectSetup(PhiBlurEffect *e, const PhiBlurConfig *cfg);

// Unloads shader
void PhiBlurEffectUninit(PhiBlurEffect *e);

// Returns default config
PhiBlurConfig PhiBlurConfigDefault(void);

// Registers modulatable params with the modulation engine
void PhiBlurRegisterParams(PhiBlurConfig *cfg);

#endif // PHI_BLUR_H
