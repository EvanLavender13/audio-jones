// Oil paint effect module
// Two-pass pipeline: brush stroke tracing at half-res, then specular composite

#ifndef OIL_PAINT_H
#define OIL_PAINT_H

#include "raylib.h"
#include <stdbool.h>

struct OilPaintConfig {
  bool enabled = false;
  float brushSize = 1.0f; // Stroke width relative to grid cell (0.5-3.0)
  float strokeBend =
      -1.0f; // Curvature bias follows/opposes gradient (-2.0-2.0)
  float brushDetail = 0.1f; // Gradient threshold for stroke culling (0.01-0.5)
  float srcContrast = 1.4f; // Source color contrast boost (0.5-3.0)
  float srcBright = 1.0f;   // Source brightness (0.5-1.5)
  float specular = 0.15f;   // Surface sheen from relief lighting (0.0-1.0)
  int layers = 8;           // Multi-scale layer count (3-11)
};

#define OIL_PAINT_CONFIG_FIELDS                                                \
  enabled, brushSize, strokeBend, brushDetail, srcContrast, srcBright,         \
      specular, layers

typedef struct OilPaintEffect {
  Shader strokeShader;
  Shader compositeShader;
  RenderTexture2D intermediate;

  // Stroke shader uniform locations
  int strokeResolutionLoc;
  int brushSizeLoc;
  int strokeBendLoc;
  int brushDetailLoc;
  int srcContrastLoc;
  int srcBrightLoc;
  int layersLoc;
  int noiseTexLoc;

  // Composite shader uniform locations
  int compositeResolutionLoc;
  int specularLoc;
} OilPaintEffect;

// Returns true on success, false if either shader fails to load
bool OilPaintEffectInit(OilPaintEffect *e, int width, int height);

// Binds all non-resolution uniforms on both stroke and composite shaders
void OilPaintEffectSetup(const OilPaintEffect *e, const OilPaintConfig *cfg,
                         float deltaTime);

// Unloads both shaders and intermediate render texture
void OilPaintEffectUninit(const OilPaintEffect *e);

// Recreates intermediate render texture at new dimensions
void OilPaintEffectResize(OilPaintEffect *e, int width, int height);

// Registers modulatable params with the modulation engine
void OilPaintRegisterParams(OilPaintConfig *cfg);

typedef struct PostEffect PostEffect;

// Runs the oil paint 2-pass pipeline at half resolution
void ApplyHalfResOilPaint(PostEffect *pe, const RenderTexture2D *source,
                          const int *writeIdx);

OilPaintEffect *GetOilPaintEffect(PostEffect *pe);

#endif // OIL_PAINT_H
