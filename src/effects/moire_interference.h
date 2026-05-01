#ifndef MOIRE_INTERFERENCE_H
#define MOIRE_INTERFERENCE_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

// Moire Interference: Per-layer wave displacement with configurable pattern and
// profile modes producing large-scale interference fringes

struct MoireInterferenceLayerConfig {
  float frequency = 10.0f;    // Wave spatial frequency (1.0-30.0)
  float angle = 0.0f;         // Wave direction in radians
  float rotationSpeed = 0.3f; // Drift rate in radians/second
  float amplitude = 0.04f;    // Displacement strength (0.0-0.15)
  float phase = 0.0f;         // Phase offset in radians
};

#define MOIRE_INTERFERENCE_LAYER_CONFIG_FIELDS                                 \
  frequency, angle, rotationSpeed, amplitude, phase

struct MoireInterferenceConfig {
  bool enabled = false;
  int patternMode = 0; // 0=stripes, 1=circles, 2=grid
  int profileMode = 0; // 0=sine, 1=square, 2=triangle, 3=sawtooth
  int layerCount = 2;  // Active layers (2-4)
  float centerX = 0.5f;
  float centerY = 0.5f;

  MoireInterferenceLayerConfig layer0;
  MoireInterferenceLayerConfig layer1 = {.frequency = 10.0f,
                                         .angle = 1.047f,
                                         .rotationSpeed = 0.3f,
                                         .amplitude = 0.04f,
                                         .phase = 0.0f};
  MoireInterferenceLayerConfig layer2 = {.frequency = 10.0f,
                                         .angle = 2.094f,
                                         .rotationSpeed = 0.3f,
                                         .amplitude = 0.04f,
                                         .phase = 0.0f};
  MoireInterferenceLayerConfig layer3 = {.frequency = 10.0f,
                                         .angle = 3.14159f,
                                         .rotationSpeed = 0.3f,
                                         .amplitude = 0.04f,
                                         .phase = 0.0f};
};

#define MOIRE_INTERFERENCE_CONFIG_FIELDS                                       \
  enabled, patternMode, profileMode, layerCount, centerX, centerY, layer0,     \
      layer1, layer2, layer3

typedef struct MoireInterferenceEffect {
  Shader shader;

  // Global uniform locations
  int patternModeLoc;
  int profileModeLoc;
  int layerCountLoc;
  int centerXLoc;
  int centerYLoc;
  int resolutionLoc;

  // Per-layer uniform locations (4 each)
  int frequencyLoc[4];
  int angleLoc[4]; // receives angle + accumulated drift
  int amplitudeLoc[4];
  int phaseLoc[4];

  // Runtime state
  float layerAngles[4]; // Per-layer rotation accumulators
} MoireInterferenceEffect;

// Returns true on success, false if shader fails to load
bool MoireInterferenceEffectInit(MoireInterferenceEffect *e);

// Accumulates per-layer rotation, sets all uniforms
void MoireInterferenceEffectSetup(MoireInterferenceEffect *e,
                                  const MoireInterferenceConfig *cfg,
                                  float deltaTime);

// Unloads shader
void MoireInterferenceEffectUninit(const MoireInterferenceEffect *e);

// Registers modulatable params with the modulation engine
void MoireInterferenceRegisterParams(MoireInterferenceConfig *cfg);

MoireInterferenceEffect *GetMoireInterferenceEffect(PostEffect *pe);

#endif // MOIRE_INTERFERENCE_H
