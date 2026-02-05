// Ink wash effect module
// Applies Sobel edge darkening, FBM paper granulation, and directional color
// bleed

#ifndef INK_WASH_H
#define INK_WASH_H

#include "raylib.h"
#include <stdbool.h>

struct InkWashConfig {
  bool enabled = false;
  float strength = 1.0f;      // Edge darkening intensity (0.0-2.0)
  float granulation = 0.5f;   // Paper noise intensity (0.0-1.0)
  float bleedStrength = 0.5f; // Directional color bleed (0.0-1.0)
  float bleedRadius = 5.0f;   // How far colors spread at edges (1.0-10.0)
  float softness = 0.0f;      // Pre-blur radius before edge detection (0.0-5.0)
};

typedef struct InkWashEffect {
  Shader shader;
  int resolutionLoc;
  int strengthLoc;
  int granulationLoc;
  int bleedStrengthLoc;
  int bleedRadiusLoc;
  int softnessLoc;
} InkWashEffect;

// Returns true on success, false if shader fails to load
bool InkWashEffectInit(InkWashEffect *e);

// Sets all uniforms including resolution and softness int cast
void InkWashEffectSetup(InkWashEffect *e, const InkWashConfig *cfg);

// Unloads shader
void InkWashEffectUninit(InkWashEffect *e);

// Returns default config
InkWashConfig InkWashConfigDefault(void);

// Registers modulatable params with the modulation engine
void InkWashRegisterParams(InkWashConfig *cfg);

#endif // INK_WASH_H
