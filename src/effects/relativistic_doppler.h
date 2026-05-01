// Relativistic Doppler: velocity-dependent color shift with headlight beaming
// Simulates relativistic aberration and frequency shift based on observer
// velocity.

#ifndef RELATIVISTIC_DOPPLER_H
#define RELATIVISTIC_DOPPLER_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

struct RelativisticDopplerConfig {
  bool enabled = false;
  float velocity = 0.5f;
  float centerX = 0.5f;
  float centerY = 0.5f;
  float aberration = 1.0f;
  float colorShift = 1.0f;
  float headlight = 0.3f;
};

#define RELATIVISTIC_DOPPLER_CONFIG_FIELDS                                     \
  enabled, velocity, centerX, centerY, aberration, colorShift, headlight

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
void RelativisticDopplerEffectSetup(const RelativisticDopplerEffect *e,
                                    const RelativisticDopplerConfig *cfg,
                                    float deltaTime);

// Unloads shader
void RelativisticDopplerEffectUninit(const RelativisticDopplerEffect *e);

// Registers modulatable params with the modulation engine
void RelativisticDopplerRegisterParams(RelativisticDopplerConfig *cfg);

RelativisticDopplerEffect *GetRelativisticDopplerEffect(PostEffect *pe);

#endif // RELATIVISTIC_DOPPLER_H
