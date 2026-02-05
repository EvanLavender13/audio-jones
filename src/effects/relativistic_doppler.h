// Relativistic Doppler: velocity-dependent color shift with headlight beaming
// Simulates relativistic aberration and frequency shift based on observer
// velocity.

#ifndef RELATIVISTIC_DOPPLER_H
#define RELATIVISTIC_DOPPLER_H

#include "raylib.h"
#include <stdbool.h>

struct RelativisticDopplerConfig {
  bool enabled = false;
  float velocity = 0.5f;
  float centerX = 0.5f;
  float centerY = 0.5f;
  float aberration = 1.0f;
  float colorShift = 1.0f;
  float headlight = 0.3f;
};

typedef struct RelativisticDopplerEffect {
  Shader shader;
  int resolutionLoc;
  int velocityLoc;
  int centerLoc;
  int aberrationLoc;
  int colorShiftLoc;
  int headlightLoc;
} RelativisticDopplerEffect;

// Returns true on success, false if shader fails to load
bool RelativisticDopplerEffectInit(RelativisticDopplerEffect *e);

// Sets all uniforms from config values
void RelativisticDopplerEffectSetup(RelativisticDopplerEffect *e,
                                    const RelativisticDopplerConfig *cfg,
                                    float deltaTime);

// Unloads shader
void RelativisticDopplerEffectUninit(RelativisticDopplerEffect *e);

// Returns default config
RelativisticDopplerConfig RelativisticDopplerConfigDefault(void);

// Registers modulatable params with the modulation engine
void RelativisticDopplerRegisterParams(RelativisticDopplerConfig *cfg);

#endif // RELATIVISTIC_DOPPLER_H
