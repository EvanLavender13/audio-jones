// Halftone effect module
// Four-color CMYK dot pattern simulating print halftoning

#ifndef HALFTONE_H
#define HALFTONE_H

#include "raylib.h"
#include <stdbool.h>

struct HalftoneConfig {
  bool enabled = false;
  float dotScale = 8.0f;      // Grid cell size in pixels (2.0-20.0)
  float dotSize = 1.0f;       // Ink density multiplier (0.5-2.0)
  float rotationSpeed = 0.0f; // Rotation rate in radians/second
  float rotationAngle = 0.0f; // Static rotation offset in radians
};

#define HALFTONE_CONFIG_FIELDS                                                 \
  enabled, dotScale, dotSize, rotationSpeed, rotationAngle

typedef struct HalftoneEffect {
  Shader shader;
  int resolutionLoc;
  int dotScaleLoc;
  int dotSizeLoc;
  int rotationLoc;
  float rotation; // Rotation accumulator
} HalftoneEffect;

// Returns true on success, false if shader fails to load
bool HalftoneEffectInit(HalftoneEffect *e);

// Accumulates rotation, sets all uniforms
void HalftoneEffectSetup(HalftoneEffect *e, const HalftoneConfig *cfg,
                         float deltaTime);

// Unloads shader
void HalftoneEffectUninit(const HalftoneEffect *e);

// Registers modulatable params with the modulation engine
void HalftoneRegisterParams(HalftoneConfig *cfg);

#endif // HALFTONE_H
