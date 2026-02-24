// Risograph effect module
// Simulates multi-layer ink printing with grain erosion and misregistration

#ifndef RISOGRAPH_H
#define RISOGRAPH_H

#include "raylib.h"
#include <stdbool.h>

struct RisographConfig {
  bool enabled = false;
  float grainScale = 200.0f;   // Noise sample scale (50-800)
  float grainIntensity = 0.4f; // Ink erosion amount (0.0-1.0)
  float grainSpeed = 0.3f;     // Grain animation rate (0.0-2.0)
  float misregAmount = 0.005f; // UV offset per ink layer (0.0-0.02)
  float misregSpeed = 0.2f;    // Misregistration drift rate (0.0-2.0)
  float inkDensity = 1.0f;     // Overall ink coverage (0.2-1.5)
  int posterize = 0;           // Tonal steps (0=off, 2-16)
  float paperTone = 0.3f;      // Paper warmth (0.0-1.0)
};

#define RISOGRAPH_CONFIG_FIELDS                                                \
  enabled, grainScale, grainIntensity, grainSpeed, misregAmount, misregSpeed,  \
      inkDensity, posterize, paperTone

typedef struct RisographEffect {
  Shader shader;
  int resolutionLoc;
  int grainScaleLoc;
  int grainIntensityLoc;
  int grainTimeLoc;
  int misregAmountLoc;
  int misregTimeLoc;
  int inkDensityLoc;
  int posterizeLoc;
  int paperToneLoc;
  float grainTime;  // grainSpeed accumulator
  float misregTime; // misregSpeed accumulator
} RisographEffect;

// Returns true on success, false if shader fails to load
bool RisographEffectInit(RisographEffect *e);

// Accumulates time, sets all uniforms
void RisographEffectSetup(RisographEffect *e, const RisographConfig *cfg,
                          float deltaTime);

// Unloads shader
void RisographEffectUninit(RisographEffect *e);

// Returns default config
RisographConfig RisographConfigDefault(void);

// Registers modulatable params with the modulation engine
void RisographRegisterParams(RisographConfig *cfg);

#endif // RISOGRAPH_H
