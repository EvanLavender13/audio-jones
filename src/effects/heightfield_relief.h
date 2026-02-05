// Heightfield Relief effect module
// Treats luminance as a heightfield with directional lighting and specular

#ifndef HEIGHTFIELD_RELIEF_H
#define HEIGHTFIELD_RELIEF_H

#include "raylib.h"
#include <stdbool.h>

struct HeightfieldReliefConfig {
  bool enabled = false;
  float intensity = 0.7f;    // Blend strength (0.0-1.0)
  float reliefScale = 0.2f;  // Surface flatness, higher = subtler (0.02-1.0)
  float lightAngle = 0.785f; // Light direction in radians (0-2pi)
  float lightHeight = 0.5f;  // Light elevation (0.1-2.0)
  float shininess = 32.0f;   // Specular exponent (1.0-128.0)
};

typedef struct HeightfieldReliefEffect {
  Shader shader;
  int resolutionLoc;
  int intensityLoc;
  int reliefScaleLoc;
  int lightAngleLoc;
  int lightHeightLoc;
  int shininessLoc;
} HeightfieldReliefEffect;

// Returns true on success, false if shader fails to load
bool HeightfieldReliefEffectInit(HeightfieldReliefEffect *e);

// Sets all uniforms
void HeightfieldReliefEffectSetup(HeightfieldReliefEffect *e,
                                  const HeightfieldReliefConfig *cfg);

// Unloads shader
void HeightfieldReliefEffectUninit(HeightfieldReliefEffect *e);

// Returns default config
HeightfieldReliefConfig HeightfieldReliefConfigDefault(void);

#endif // HEIGHTFIELD_RELIEF_H
