// Voronoi multi-effect module
// Computes voronoi geometry once, applies selected cell sub-effect

#ifndef VORONOI_H
#define VORONOI_H

#include "raylib.h"
#include <stdbool.h>

// Shared cell sub-effect modes used by Voronoi and Phyllotaxis
typedef enum {
  CELL_MODE_UV_DISTORT = 0,   // "Distort"
  CELL_MODE_ORGANIC_FLOW = 1, // "Organic Flow"
  CELL_MODE_EDGE_ISO = 2,     // "Edge Iso"
  CELL_MODE_CENTER_ISO = 3,   // "Center Iso"
  CELL_MODE_FLAT_FILL = 4,    // "Flat Fill"
  CELL_MODE_EDGE_GLOW = 5,    // "Edge Glow"
  CELL_MODE_RATIO = 6,        // "Ratio"
  CELL_MODE_DETERMINANT = 7,  // "Determinant"
  CELL_MODE_EDGE_DETECT = 8,  // "Edge Detect"
  CELL_MODE_COUNT = 9
} CellMode;

struct VoronoiConfig {
  bool enabled = false;
  bool smoothMode = false;
  float scale = 15.0f;
  float speed = 0.5f;
  float edgeFalloff = 0.3f;
  float isoFrequency = 10.0f;
  int mode = 0;           // CellMode index (0-8)
  float intensity = 0.5f; // Sub-effect intensity (0.0-1.0)
};

#define VORONOI_CONFIG_FIELDS                                                  \
  enabled, smoothMode, scale, speed, edgeFalloff, isoFrequency, mode, intensity

typedef struct VoronoiEffect {
  Shader shader;
  int resolutionLoc;
  int scaleLoc;
  int timeLoc;
  int edgeFalloffLoc;
  int isoFrequencyLoc;
  int smoothModeLoc;
  int modeLoc;
  int intensityLoc;
  float time; // Animation accumulator
} VoronoiEffect;

bool VoronoiEffectInit(VoronoiEffect *e);
void VoronoiEffectSetup(VoronoiEffect *e, const VoronoiConfig *cfg,
                        float deltaTime);
void VoronoiEffectUninit(VoronoiEffect *e);
VoronoiConfig VoronoiConfigDefault(void);
void VoronoiRegisterParams(VoronoiConfig *cfg);

#endif // VORONOI_H
