// Voronoi multi-effect module
// Computes voronoi geometry once, applies multiple blendable effects

#ifndef VORONOI_H
#define VORONOI_H

#include "raylib.h"
#include <stdbool.h>

struct VoronoiConfig {
  bool enabled = false;
  bool smoothMode = false;
  float scale = 15.0f;
  float speed = 0.5f;
  float edgeFalloff = 0.3f;
  float isoFrequency = 10.0f;
  float uvDistortIntensity = 0.0f;
  float edgeIsoIntensity = 0.0f;
  float centerIsoIntensity = 0.0f;
  float flatFillIntensity = 0.0f;
  float organicFlowIntensity = 0.0f;
  float edgeGlowIntensity = 0.0f;
  float determinantIntensity = 0.0f;
  float ratioIntensity = 0.0f;
  float edgeDetectIntensity = 0.0f;
};

typedef struct VoronoiEffect {
  Shader shader;
  int resolutionLoc;
  int scaleLoc;
  int timeLoc;
  int edgeFalloffLoc;
  int isoFrequencyLoc;
  int smoothModeLoc;
  int uvDistortIntensityLoc;
  int edgeIsoIntensityLoc;
  int centerIsoIntensityLoc;
  int flatFillIntensityLoc;
  int organicFlowIntensityLoc;
  int edgeGlowIntensityLoc;
  int determinantIntensityLoc;
  int ratioIntensityLoc;
  int edgeDetectIntensityLoc;
  float time; // Animation accumulator
} VoronoiEffect;

bool VoronoiEffectInit(VoronoiEffect *e);
void VoronoiEffectSetup(VoronoiEffect *e, const VoronoiConfig *cfg,
                        float deltaTime);
void VoronoiEffectUninit(VoronoiEffect *e);
VoronoiConfig VoronoiConfigDefault(void);
void VoronoiRegisterParams(VoronoiConfig *cfg);

#endif // VORONOI_H
