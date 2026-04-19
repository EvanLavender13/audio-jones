// Drekker Paint painterly effect: geometric paint-stroke mosaic with diagonal
// curved parallelogram cells

#ifndef DREKKER_PAINT_H
#define DREKKER_PAINT_H

#include "raylib.h"
#include <stdbool.h>

struct DrekkerPaintConfig {
  bool enabled = false;
  float xDiv = 11.0f; // Horizontal cell count (2-30)
  float yDiv = 7.0f;  // Vertical cell count (2-20)
  float curve =
      0.01f; // Reciprocal curvature intensity at cell edges (0.001-0.1)
  float gapSize =
      0.1f; // Alpha threshold for visible gap between strokes (0.01-0.4)
  float diagSlant = 0.1f; // Diagonal slant factor, rx multiplier (0.0-0.3)
  float strokeSpread =
      200.0f; // Vertical stroke sample spread, pixels at 1080p (50-500)
};

#define DREKKER_PAINT_CONFIG_FIELDS                                            \
  enabled, xDiv, yDiv, curve, gapSize, diagSlant, strokeSpread

typedef struct DrekkerPaintEffect {
  Shader shader;
  int resolutionLoc;
  int xDivLoc;
  int yDivLoc;
  int curveLoc;
  int gapSizeLoc;
  int diagSlantLoc;
  int strokeSpreadLoc;
} DrekkerPaintEffect;

// Returns true on success, false if shader fails to load
bool DrekkerPaintEffectInit(DrekkerPaintEffect *e);

// Sets all uniforms
void DrekkerPaintEffectSetup(const DrekkerPaintEffect *e,
                             const DrekkerPaintConfig *cfg);

// Unloads shader
void DrekkerPaintEffectUninit(const DrekkerPaintEffect *e);

// Registers modulatable params with the modulation engine
void DrekkerPaintRegisterParams(DrekkerPaintConfig *cfg);

#endif // DREKKER_PAINT_H
