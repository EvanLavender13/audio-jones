#ifndef FRACTURE_GRID_H
#define FRACTURE_GRID_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

// Fracture Grid - subdivides the image into tiles with per-tile UV offset,
// rotation, and zoom driven by a stagger hash for a shattered mosaic look.
struct FractureGridConfig {
  bool enabled = false;
  float subdivision = 4.0f;   // 0.0-20.0 - grid density
  float stagger = 0.5f;       // 0.0-1.0 - per-tile variation intensity
  float offsetScale = 0.3f;   // 0.0-1.0 - max UV offset per tile
  float rotationScale = 0.5f; // -PI-PI - max rotation per tile (radians)
  float zoomScale = 1.0f;     // 0.0-4.0 - max zoom deviation per tile
  int tessellation = 0;       // 0=rect, 1=hex, 2=triangular
  float waveSpeed = 1.0f;     // 0.0-5.0 - wave travel speed
  float waveShape =
      0.0f; // 0.0-1.0 - smooth sine (0) to snappy hold-and-release (1)
  float spatialBias =
      0.0f; // 0.0-1.0 - random hash per tile (0) to position-derived (1)
  float flipChance = 0.0f;       // 0.0-1.0 - probability of per-tile h/v flip
  float skewScale = 0.0f;        // 0.0-2.0 - max shear intensity per tile
  int propagationMode = 0;       // 0=noise, 1=directional, 2=radial, 3=cascade
  float propagationSpeed = 5.0f; // 0.0-20.0 - phase offset scale from distance
  float propagationAngle = 0.0f; // -PI..PI - sweep direction (radians, mode 1)
};

#define FRACTURE_GRID_CONFIG_FIELDS                                            \
  enabled, subdivision, stagger, offsetScale, rotationScale, zoomScale,        \
      tessellation, waveSpeed, waveShape, spatialBias, flipChance, skewScale,  \
      propagationMode, propagationSpeed, propagationAngle

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
  int waveShapeLoc;
  int spatialBiasLoc;
  int flipChanceLoc;
  int skewScaleLoc;
  int propagationModeLoc;
  int propagationSpeedLoc;
  int propagationAngleLoc;
  int propagationPhaseLoc;
  float waveTime;
  float propagationPhase;
} FractureGridEffect;

// Returns true on success, false if shader fails to load
bool FractureGridEffectInit(FractureGridEffect *e);

// Accumulates wave time, sets all uniforms
void FractureGridEffectSetup(FractureGridEffect *e,
                             const FractureGridConfig *cfg, float deltaTime,
                             int screenWidth, int screenHeight);

// Unloads shader
void FractureGridEffectUninit(const FractureGridEffect *e);

// Registers modulatable params with the modulation engine
void FractureGridRegisterParams(FractureGridConfig *cfg);

FractureGridEffect *GetFractureGridEffect(PostEffect *pe);

#endif // FRACTURE_GRID_H
