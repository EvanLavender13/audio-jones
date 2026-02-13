// Phi Blur effect module
// Golden-ratio sampled blur with rectangular and disc kernel modes

#ifndef PHI_BLUR_H
#define PHI_BLUR_H

#include "raylib.h"
#include <stdbool.h>

// PhiBlur: Golden-ratio distributed blur samples with configurable kernel shape
// Supports rectangular (rotatable, aspect-adjustable) and disc modes
struct PhiBlurConfig {
  bool enabled = false;
  int mode = 0;             // 0 = Rect, 1 = Disc
  float radius = 5.0f;      // Blur extent in pixels (0.0-50.0)
  float angle = 0.0f;       // Rect kernel rotation in radians (0-2pi)
  float aspectRatio = 1.0f; // Rect kernel width/height ratio (0.1-10.0)
  int samples = 32;         // Sample count (8-128)
  float gamma = 2.2f;       // Blending gamma (1.0-6.0)
};

typedef struct PhiBlurEffect {
  Shader shader;
  int resolutionLoc;
  int modeLoc;
  int radiusLoc;
  int angleLoc;
  int aspectRatioLoc;
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
