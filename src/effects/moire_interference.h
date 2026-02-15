#ifndef MOIRE_INTERFERENCE_H
#define MOIRE_INTERFERENCE_H

#include "raylib.h"
#include <stdbool.h>

// Moire Interference: Multi-sample UV transform with rotated/scaled copies
// Small rotation/scale differences produce large-scale wave interference
// patterns

struct MoireInterferenceConfig {
  bool enabled = false;
  float rotationAngle = 0.087f;  // Angle between layers (radians, ~5 deg)
  float scaleDiff = 1.02f;       // Scale ratio between layers
  int layers = 2;                // Number of overlaid samples (2-4)
  int blendMode = 0;             // 0=multiply, 1=min, 2=average, 3=difference
  float centerX = 0.5f;          // Rotation/scale center X
  float centerY = 0.5f;          // Rotation/scale center Y
  float animationSpeed = 0.017f; // Rotation rate (radians/second, ~1 deg/s)
};

#define MOIRE_INTERFERENCE_CONFIG_FIELDS                                       \
  enabled, rotationAngle, scaleDiff, layers, blendMode, centerX, centerY,      \
      animationSpeed

typedef struct MoireInterferenceEffect {
  Shader shader;
  int rotationAngleLoc;
  int scaleDiffLoc;
  int layersLoc;
  int blendModeLoc;
  int centerXLoc;
  int centerYLoc;
  int rotationAccumLoc;
  int resolutionLoc;
  float rotationAccum; // Animation accumulator
} MoireInterferenceEffect;

// Returns true on success, false if shader fails to load
bool MoireInterferenceEffectInit(MoireInterferenceEffect *e);

// Accumulates rotation, sets all uniforms
void MoireInterferenceEffectSetup(MoireInterferenceEffect *e,
                                  const MoireInterferenceConfig *cfg,
                                  float deltaTime);

// Unloads shader
void MoireInterferenceEffectUninit(MoireInterferenceEffect *e);

// Returns default config
MoireInterferenceConfig MoireInterferenceConfigDefault(void);

// Registers modulatable params with the modulation engine
void MoireInterferenceRegisterParams(MoireInterferenceConfig *cfg);

#endif // MOIRE_INTERFERENCE_H
