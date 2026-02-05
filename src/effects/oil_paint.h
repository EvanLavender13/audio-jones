// Oil paint effect module
// Two-pass pipeline: brush stroke tracing at half-res, then specular composite

#ifndef OIL_PAINT_H
#define OIL_PAINT_H

#include "raylib.h"
#include <stdbool.h>

struct OilPaintConfig {
  bool enabled = false;
  float brushSize = 1.0f;   // Stroke width relative to base grid cell (0.5-3.0)
  float strokeBend = -1.0f; // Curvature bias follows or opposes gradient
                            // direction (-2.0 to 2.0)
  float specular =
      0.15f; // Surface sheen from simulated paint thickness variation (0.0-1.0)
  int layers = 8; // Overlapping passes blend like wet-on-wet technique (3-11)
};

typedef struct OilPaintEffect {
  Shader strokeShader;
  Shader compositeShader;
  Texture2D noiseTex;
  RenderTexture2D intermediate;

  // Stroke shader uniform locations
  int strokeResolutionLoc;
  int brushSizeLoc;
  int strokeBendLoc;
  int layersLoc;
  int noiseTexLoc;

  // Composite shader uniform locations
  int compositeResolutionLoc;
  int specularLoc;
} OilPaintEffect;

// Returns true on success, false if either shader fails to load
bool OilPaintEffectInit(OilPaintEffect *e, int width, int height);

// Sets uniforms on composite shader (stroke uniforms set by
// ApplyHalfResOilPaint)
void OilPaintEffectSetup(OilPaintEffect *e, const OilPaintConfig *cfg);

// Unloads both shaders, noise texture, and intermediate render texture
void OilPaintEffectUninit(OilPaintEffect *e);

// Recreates intermediate render texture at new dimensions
void OilPaintEffectResize(OilPaintEffect *e, int width, int height);

// Returns default config
OilPaintConfig OilPaintConfigDefault(void);

// Registers modulatable params with the modulation engine
void OilPaintRegisterParams(OilPaintConfig *cfg);

#endif // OIL_PAINT_H
