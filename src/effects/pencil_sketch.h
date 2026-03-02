// Pencil sketch effect module
// Directional gradient-aligned stroke accumulation with paper texture

#ifndef PENCIL_SKETCH_H
#define PENCIL_SKETCH_H

#include "raylib.h"
#include <stdbool.h>

struct PencilSketchConfig {
  bool enabled = false;
  int angleCount = 3;          // Number of hatching directions (2-6)
  int sampleCount = 16;        // Samples per direction/stroke length (8-24)
  float gradientEps = 0.4f;    // Edge sensitivity epsilon (0.2-1.0)
  float desaturation = 1.8f;   // Color muting toward gray (0.0-3.0)
  float toneCap = 0.7f;        // Max brightness clamp (0.3-1.0)
  float noiseInfluence = 0.8f; // Noise modulation of stroke intensity (0.0-1.0)
  float colorStrength = 1.0f;  // Monochrome (0) to tinted strokes (1) (0.0-1.0)
  float paperStrength = 1.0f;  // Paper texture visibility (0.0-1.0)
  float vignetteStrength = 1.0f; // Edge darkening (0.0-1.0)
  float wobbleSpeed = 1.0f;      // Animation rate, 0 = static (0.0-2.0)
  float wobbleAmount = 4.0f;     // Pixel displacement magnitude (0.0-8.0)
};

#define PENCIL_SKETCH_CONFIG_FIELDS                                            \
  enabled, angleCount, sampleCount, gradientEps, desaturation, toneCap,        \
      noiseInfluence, colorStrength, paperStrength, vignetteStrength,          \
      wobbleSpeed, wobbleAmount

typedef struct PencilSketchEffect {
  Shader shader;
  int resolutionLoc;
  int angleCountLoc;
  int sampleCountLoc;
  int gradientEpsLoc;
  int desaturationLoc;
  int toneCapLoc;
  int noiseInfluenceLoc;
  int colorStrengthLoc;
  int paperStrengthLoc;
  int vignetteStrengthLoc;
  int wobbleTimeLoc;
  int wobbleAmountLoc;
  int noiseTexLoc;
  float wobbleTime;
} PencilSketchEffect;

// Returns true on success, false if shader fails to load
bool PencilSketchEffectInit(PencilSketchEffect *e);

// Accumulates wobble time, sets all uniforms including resolution
void PencilSketchEffectSetup(PencilSketchEffect *e,
                             const PencilSketchConfig *cfg, float deltaTime);

// Unloads shader
void PencilSketchEffectUninit(PencilSketchEffect *e);

// Returns default config
PencilSketchConfig PencilSketchConfigDefault(void);

// Registers modulatable params with the modulation engine
void PencilSketchRegisterParams(PencilSketchConfig *cfg);

#endif // PENCIL_SKETCH_H
