// Dot matrix effect module
// Grid-quantized inverse-cube glow dots with rotation

#ifndef DOT_MATRIX_H
#define DOT_MATRIX_H

#include "raylib.h"
#include <stdbool.h>

struct DotMatrixConfig {
  bool enabled = false;
  float dotScale = 32.0f;     // Grid resolution (4.0-80.0)
  float softness = 1.2f;      // Glow falloff tightness (0.2-4.0)
  float brightness = 3.0f;    // Dot intensity multiplier (0.5-8.0)
  float rotationSpeed = 0.0f; // Grid rotation rate in radians/second
  float rotationAngle = 0.0f; // Static rotation offset in radians
};

#define DOT_MATRIX_CONFIG_FIELDS                                               \
  enabled, dotScale, softness, brightness, rotationSpeed, rotationAngle

typedef struct DotMatrixEffect {
  Shader shader;
  int resolutionLoc;
  int dotScaleLoc;
  int softnessLoc;
  int brightnessLoc;
  int rotationLoc;
  float rotation; // Rotation accumulator
} DotMatrixEffect;

// Returns true on success, false if shader fails to load
bool DotMatrixEffectInit(DotMatrixEffect *e);

// Accumulates rotation, sets all uniforms
void DotMatrixEffectSetup(DotMatrixEffect *e, const DotMatrixConfig *cfg,
                          float deltaTime);

// Unloads shader
void DotMatrixEffectUninit(DotMatrixEffect *e);

// Returns default config
DotMatrixConfig DotMatrixConfigDefault(void);

// Registers modulatable params with the modulation engine
void DotMatrixRegisterParams(DotMatrixConfig *cfg);

#endif // DOT_MATRIX_H
