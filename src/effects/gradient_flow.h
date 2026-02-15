#ifndef GRADIENT_FLOW_H
#define GRADIENT_FLOW_H

#include "raylib.h"
#include <stdbool.h>

// Gradient Flow distortion effect
// Displaces pixels along luminance gradient tangents. Creates organic flow
// patterns by iteratively sliding pixels perpendicular to brightness edges.
struct GradientFlowConfig {
  bool enabled = false;
  float strength = 0.01f; // Displacement per iteration (0.0 to 0.1)
  int iterations = 8;     // Cascade depth (1 to 8)
  float edgeWeight =
      1.0f; // Blend between uniform (0) and edge-scaled (1) displacement
  bool randomDirection =
      false; // Randomize tangent direction per pixel for crunchy look
};

#define GRADIENT_FLOW_CONFIG_FIELDS                                            \
  enabled, strength, iterations, edgeWeight, randomDirection

typedef struct GradientFlowEffect {
  Shader shader;
  int resolutionLoc;
  int strengthLoc;
  int iterationsLoc;
  int edgeWeightLoc;
  int randomDirectionLoc;
} GradientFlowEffect;

// Returns true on success, false if shader fails to load
bool GradientFlowEffectInit(GradientFlowEffect *e);

// Sets all uniforms for current frame
void GradientFlowEffectSetup(GradientFlowEffect *e,
                             const GradientFlowConfig *cfg, int screenWidth,
                             int screenHeight);

// Unloads shader
void GradientFlowEffectUninit(GradientFlowEffect *e);

// Returns default config
GradientFlowConfig GradientFlowConfigDefault(void);

// Registers modulatable params with the modulation engine
void GradientFlowRegisterParams(GradientFlowConfig *cfg);

#endif // GRADIENT_FLOW_H
