#ifndef ESCHER_DROSTE_H
#define ESCHER_DROSTE_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

// Escher-style self-tiling Droste warp
// Log-polar recursive tiling distinct from the spiral-zoom droste. Maps the
// plane through complex logarithm, tiles in log-radius/angle space, then maps
// back via exponential to produce self-similar Escher-style recursion.
struct EscherDrosteConfig {
  bool enabled = false;
  float scale = 256.0f;   // Tile scale factor k (4.0 - 1024.0, log slider)
  float zoomSpeed = 0.5f; // Log-radial zoom rate, signed (-2.0 - 2.0)
  float spiralStrength =
      1.0f; // 0 = zoom-only Droste, 1 = Escher spiral (-2.0 - 2.0)
  float rotationOffset =
      0.0f; // Static rotation of tiling pattern, radians (-PI - PI)
  DualLissajousConfig center = {.amplitude =
                                    0.0f}; // 2D drift of vanishing point
  float innerRadius = 0.05f; // Fade mask around singular center (0.0 - 0.5)
};

#define ESCHER_DROSTE_CONFIG_FIELDS                                            \
  enabled, scale, zoomSpeed, spiralStrength, rotationOffset, center, innerRadius

typedef struct EscherDrosteEffect {
  Shader shader;
  int resolutionLoc;
  int centerLoc;
  int scaleLoc;
  int zoomPhaseLoc;
  int spiralStrengthLoc;
  int rotationOffsetLoc;
  int innerRadiusLoc;
  float zoomPhase; // CPU-accumulated phase, replaces iTime*0.5 from reference
} EscherDrosteEffect;

// Returns true on success, false if shader fails to load
bool EscherDrosteEffectInit(EscherDrosteEffect *e);

// Accumulates zoom phase, updates embedded lissajous, sets all uniforms
void EscherDrosteEffectSetup(EscherDrosteEffect *e, EscherDrosteConfig *cfg,
                             float deltaTime);

// Unloads shader
void EscherDrosteEffectUninit(const EscherDrosteEffect *e);

// Registers modulatable params with the modulation engine
void EscherDrosteRegisterParams(EscherDrosteConfig *cfg);

EscherDrosteEffect *GetEscherDrosteEffect(PostEffect *pe);

#endif // ESCHER_DROSTE_H
