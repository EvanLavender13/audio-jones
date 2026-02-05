#ifndef DROSTE_ZOOM_H
#define DROSTE_ZOOM_H

#include "raylib.h"
#include <stdbool.h>

// Droste Zoom with spiral mapping
// Applies recursive self-similar zoom using complex logarithmic mapping.
// spiralAngle controls the twist between zoom layers, shearCoeff skews the
// mapping, and branches sets the rotational symmetry count.
struct DrosteZoomConfig {
  bool enabled = false;
  float speed = 1.0f;       // Animation rate (radians/second)
  float scale = 2.5f;       // Zoom scale factor per layer
  float spiralAngle = 0.0f; // Twist between layers (radians)
  float shearCoeff = 0.0f;  // Mapping skew coefficient
  float innerRadius = 0.0f; // Inner cutoff radius (0.0-1.0)
  int branches = 1;         // Rotational symmetry count (1-8)
};

typedef struct DrosteZoomEffect {
  Shader shader;
  int timeLoc;
  int scaleLoc;
  int spiralAngleLoc;
  int shearCoeffLoc;
  int innerRadiusLoc;
  int branchesLoc;
  float time; // Animation accumulator
} DrosteZoomEffect;

// Returns true on success, false if shader fails to load
bool DrosteZoomEffectInit(DrosteZoomEffect *e);

// Accumulates time, sets all uniforms
void DrosteZoomEffectSetup(DrosteZoomEffect *e, const DrosteZoomConfig *cfg,
                           float deltaTime);

// Unloads shader
void DrosteZoomEffectUninit(DrosteZoomEffect *e);

// Returns default config
DrosteZoomConfig DrosteZoomConfigDefault(void);

// Registers modulatable params with the modulation engine
void DrosteZoomRegisterParams(DrosteZoomConfig *cfg);

#endif // DROSTE_ZOOM_H
