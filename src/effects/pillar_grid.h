// Pillar Grid: Top-down grid of rounded pillars with luminance-driven height

#ifndef PILLAR_GRID_EFFECT_H
#define PILLAR_GRID_EFFECT_H

#include "raylib.h"
#include <stdbool.h>

struct PillarGridConfig {
  bool enabled = false;
  float density = 48.0f; // Cells across the field (48-128)
  float pitch =
      1.5708f; // Camera pitch in radians (0=horizontal, HALF_PI=overhead)
  float heightScale = 8.0f; // Pillar height multiplier on cell luminance (0-8)
  float cornerRadius =
      0.3f; // Rounded-box corner softness (0=square, 0.5=spherical)
};

#define PILLAR_GRID_CONFIG_FIELDS                                              \
  enabled, density, pitch, heightScale, cornerRadius

typedef struct PillarGridEffect {
  Shader shader;
  int resolutionLoc;
  int densityLoc;
  int pitchLoc;
  int heightScaleLoc;
  int cornerRadiusLoc;
} PillarGridEffect;

bool PillarGridEffectInit(PillarGridEffect *e);
void PillarGridEffectSetup(const PillarGridEffect *e,
                           const PillarGridConfig *cfg);
void PillarGridEffectUninit(const PillarGridEffect *e);
void PillarGridRegisterParams(PillarGridConfig *cfg);

struct PostEffect;
PillarGridEffect *GetPillarGridEffect(PostEffect *pe);

#endif // PILLAR_GRID_EFFECT_H
