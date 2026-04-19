// Solarize effect module
// Inverts tones above a threshold, producing a Sabattier-style partial negative

#ifndef SOLARIZE_H
#define SOLARIZE_H

#include "raylib.h"
#include <stdbool.h>

struct SolarizeConfig {
  bool enabled = false;
  float amount = 0.5f;    // Inversion strength (0.0-1.0)
  float threshold = 0.5f; // Tone inversion peak position (0.0-1.0)
};

#define SOLARIZE_CONFIG_FIELDS enabled, amount, threshold

typedef struct SolarizeEffect {
  Shader shader;
  int amountLoc;
  int thresholdLoc;
} SolarizeEffect;

// Returns true on success, false if shader fails to load
bool SolarizeEffectInit(SolarizeEffect *e);

// Binds all uniforms for the current frame
void SolarizeEffectSetup(const SolarizeEffect *e, const SolarizeConfig *cfg);

// Unloads shader
void SolarizeEffectUninit(const SolarizeEffect *e);

// Registers modulatable params with the modulation engine
void SolarizeRegisterParams(SolarizeConfig *cfg);

#endif // SOLARIZE_H
