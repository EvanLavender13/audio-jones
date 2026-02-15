// Toon cartoon-style effect module
// Luminance posterization with Sobel edge outlines and noise-varied thickness

#ifndef TOON_H
#define TOON_H

#include "raylib.h"
#include <stdbool.h>

struct ToonConfig {
  bool enabled = false;
  int levels = 4;                  // Luminance quantization levels (2-16)
  float edgeThreshold = 0.2f;      // Edge detection sensitivity (0.0-1.0)
  float edgeSoftness = 0.05f;      // Edge antialiasing width (0.0-0.2)
  float thicknessVariation = 0.0f; // Noise-based stroke variation (0.0-1.0)
  float noiseScale = 5.0f;         // Brush stroke noise frequency (1.0-20.0)
};

#define TOON_CONFIG_FIELDS                                                     \
  enabled, levels, edgeThreshold, edgeSoftness, thicknessVariation, noiseScale

typedef struct ToonEffect {
  Shader shader;
  int resolutionLoc;
  int levelsLoc;
  int edgeThresholdLoc;
  int edgeSoftnessLoc;
  int thicknessVariationLoc;
  int noiseScaleLoc;
} ToonEffect;

// Returns true on success, false if shader fails to load
bool ToonEffectInit(ToonEffect *e);

// Sets all uniforms
void ToonEffectSetup(ToonEffect *e, const ToonConfig *cfg);

// Unloads shader
void ToonEffectUninit(ToonEffect *e);

// Returns default config
ToonConfig ToonConfigDefault(void);

// Registers modulatable params with the modulation engine
void ToonRegisterParams(ToonConfig *cfg);

#endif // TOON_H
