// Chromatic aberration effect module
// RGB channel separation with spectral sampling and distance falloff

#ifndef CHROMATIC_ABERRATION_H
#define CHROMATIC_ABERRATION_H

#include "raylib.h"
#include <stdbool.h>

// Chromatic Aberration: Separates RGB channels with configurable spectral
// sampling and radial distance falloff
struct ChromaticAberrationConfig {
  bool enabled = false;
  float offset = 0.0f;  // Channel separation in pixels (0-50)
  float samples = 3.0f; // Spectral sample count (3-24), float for modulation
  float falloff = 1.0f; // Distance curve exponent (0.5-3.0)
};

#define CHROMATIC_ABERRATION_CONFIG_FIELDS enabled, offset, samples, falloff

typedef struct ChromaticAberrationEffect {
  Shader shader;
  int resolutionLoc;
  int offsetLoc;
  int samplesLoc;
  int falloffLoc;
} ChromaticAberrationEffect;

// Returns true on success, false if shader fails to load
bool ChromaticAberrationEffectInit(ChromaticAberrationEffect *e);

// Sets all uniforms
void ChromaticAberrationEffectSetup(ChromaticAberrationEffect *e,
                                    const ChromaticAberrationConfig *cfg);

// Unloads shader
void ChromaticAberrationEffectUninit(ChromaticAberrationEffect *e);

// Returns default config
ChromaticAberrationConfig ChromaticAberrationConfigDefault(void);

// Registers modulatable params with the modulation engine
void ChromaticAberrationRegisterParams(ChromaticAberrationConfig *cfg);

#endif // CHROMATIC_ABERRATION_H
