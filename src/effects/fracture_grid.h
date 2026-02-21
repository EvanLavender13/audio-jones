#ifndef FRACTURE_GRID_H
#define FRACTURE_GRID_H

#include "raylib.h"
#include <stdbool.h>

// Fracture Grid — subdivides the image into tiles with per-tile UV offset,
// rotation, and zoom driven by a stagger hash for a shattered mosaic look.
struct FractureGridConfig {
  bool enabled = false;
  float subdivision = 4.0f;   // 0.0-20.0 — grid density
  float stagger = 0.5f;       // 0.0-1.0 — per-tile variation intensity
  float offsetScale = 0.3f;   // 0.0-1.0 — max UV offset per tile
  float rotationScale = 0.5f; // 0.0-PI — max rotation per tile (radians)
  float zoomScale = 1.0f;     // 0.0-4.0 — max zoom deviation per tile
  int tessellation = 0;       // 0=rect, 1=hex, 2=triangular
  float waveSpeed = 1.0f;     // 0.0-5.0 — wave travel speed
};

#define FRACTURE_GRID_CONFIG_FIELDS                                            \
  enabled, subdivision, stagger, offsetScale, rotationScale, zoomScale,        \
      tessellation, waveSpeed

typedef struct FractureGridEffect {
  Shader shader;
  int resolutionLoc;
  int subdivisionLoc;
  int staggerLoc;
  int offsetScaleLoc;
  int rotationScaleLoc;
  int zoomScaleLoc;
  int tessellationLoc;
  int waveTimeLoc;
  float waveTime;
} FractureGridEffect;

// Returns true on success, false if shader fails to load
bool FractureGridEffectInit(FractureGridEffect *e);

// Accumulates wave time, sets all uniforms
void FractureGridEffectSetup(FractureGridEffect *e,
                             const FractureGridConfig *cfg, float deltaTime,
                             int screenWidth, int screenHeight);

// Unloads shader
void FractureGridEffectUninit(FractureGridEffect *e);

// Returns default config
FractureGridConfig FractureGridConfigDefault(void);

// Registers modulatable params with the modulation engine
void FractureGridRegisterParams(FractureGridConfig *cfg);

#endif // FRACTURE_GRID_H
