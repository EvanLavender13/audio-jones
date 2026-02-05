#ifndef RADIAL_STREAK_H
#define RADIAL_STREAK_H

#include "raylib.h"
#include <stdbool.h>

// Radial Streak
// Blurs pixels outward from the screen center along radial lines.
// streakLength controls how far samples reach; more samples yield smoother
// streaks at higher GPU cost.
struct RadialStreakConfig {
  bool enabled = false;
  int samples = 16;          // Number of blur taps (1-64)
  float streakLength = 0.3f; // Radial reach per tap (0.0-1.0)
};

typedef struct RadialStreakEffect {
  Shader shader;
  int samplesLoc;
  int streakLengthLoc;
} RadialStreakEffect;

// Returns true on success, false if shader fails to load
bool RadialStreakEffectInit(RadialStreakEffect *e);

// Sets all uniforms
void RadialStreakEffectSetup(RadialStreakEffect *e,
                             const RadialStreakConfig *cfg, float deltaTime);

// Unloads shader
void RadialStreakEffectUninit(RadialStreakEffect *e);

// Returns default config
RadialStreakConfig RadialStreakConfigDefault(void);

// Registers modulatable params with the modulation engine
void RadialStreakRegisterParams(RadialStreakConfig *cfg);

#endif // RADIAL_STREAK_H
