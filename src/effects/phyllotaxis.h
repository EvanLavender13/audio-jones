// Phyllotaxis cellular transform module
// Sunflower seed spiral patterns using Vogel's model

#ifndef PHYLLOTAXIS_H
#define PHYLLOTAXIS_H

#include "raylib.h"
#include <stdbool.h>

struct PhyllotaxisConfig {
  bool enabled = false;
  bool smoothMode = false;
  float scale = 0.06f;
  float divergenceAngle = 0.0f;
  float angleSpeed = 0.0f;
  float phaseSpeed = 0.0f;
  float spinSpeed = 0.0f;
  float cellRadius = 0.8f;
  float isoFrequency = 5.0f;
  float uvDistortIntensity = 0.0f;
  float organicFlowIntensity = 0.0f;
  float edgeIsoIntensity = 0.0f;
  float centerIsoIntensity = 0.0f;
  float flatFillIntensity = 0.0f;
  float edgeGlowIntensity = 0.0f;
  float ratioIntensity = 0.0f;
  float determinantIntensity = 0.0f;
  float edgeDetectIntensity = 0.0f;
};

typedef struct PhyllotaxisEffect {
  Shader shader;
  int resolutionLoc;
  int smoothModeLoc;
  int scaleLoc;
  int divergenceAngleLoc;
  int phaseTimeLoc;
  int cellRadiusLoc;
  int isoFrequencyLoc;
  int uvDistortIntensityLoc;
  int organicFlowIntensityLoc;
  int edgeIsoIntensityLoc;
  int centerIsoIntensityLoc;
  int flatFillIntensityLoc;
  int edgeGlowIntensityLoc;
  int ratioIntensityLoc;
  int determinantIntensityLoc;
  int edgeDetectIntensityLoc;
  int spinOffsetLoc;
  float angleTime;  // Divergence angle drift accumulator
  float phaseTime;  // Per-cell pulse animation accumulator
  float spinOffset; // Mechanical spin accumulator
} PhyllotaxisEffect;

// GOLDEN_ANGLE constant: 2.39996322972865f (used in Setup)

bool PhyllotaxisEffectInit(PhyllotaxisEffect *e);
void PhyllotaxisEffectSetup(PhyllotaxisEffect *e, const PhyllotaxisConfig *cfg,
                            float deltaTime);
void PhyllotaxisEffectUninit(PhyllotaxisEffect *e);
PhyllotaxisConfig PhyllotaxisConfigDefault(void);
void PhyllotaxisRegisterParams(PhyllotaxisConfig *cfg);

#endif // PHYLLOTAXIS_H
