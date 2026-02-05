// Shake effect module
// Applies screen shake distortion with configurable intensity and sampling

#ifndef SHAKE_H
#define SHAKE_H

#include "raylib.h"
#include <stdbool.h>

struct ShakeConfig {
  bool enabled = false;
  float intensity = 0.02f;
  float samples = 4.0f;
  float rate = 12.0f;
  bool gaussian = false;
};

typedef struct ShakeEffect {
  Shader shader;
  int resolutionLoc;
  int timeLoc;
  int intensityLoc;
  int samplesLoc;
  int rateLoc;
  int gaussianLoc;
  float time;
} ShakeEffect;

// Returns true on success, false if shader fails to load
bool ShakeEffectInit(ShakeEffect *e);

// Accumulates time, sets all uniforms including resolution
void ShakeEffectSetup(ShakeEffect *e, const ShakeConfig *cfg, float deltaTime);

// Unloads shader
void ShakeEffectUninit(ShakeEffect *e);

// Returns default config
ShakeConfig ShakeConfigDefault(void);

#endif // SHAKE_H
