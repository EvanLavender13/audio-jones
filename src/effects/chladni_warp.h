#ifndef CHLADNI_WARP_H
#define CHLADNI_WARP_H

#include "raylib.h"
#include <stdbool.h>

// Chladni Warp effect
// Applies coordinate displacement based on Chladni plate vibration patterns.
// Parameters n and m control the modal frequencies, creating resonant nodal
// lines.
struct ChladniWarpConfig {
  bool enabled = false;
  float n = 4.0f;         // First modal frequency
  float m = 3.0f;         // Second modal frequency
  float plateSize = 1.0f; // Virtual plate scale
  float strength = 0.1f;  // Warp displacement intensity
  int warpMode = 0;       // Displacement mode (0=default)
  float speed = 0.0f;     // Animation rate (radians/second)
  float animRange = 0.0f; // Animation oscillation range
  bool preFold = false;   // Apply coordinate folding before warp
};

typedef struct ChladniWarpEffect {
  Shader shader;
  int nLoc;
  int mLoc;
  int plateSizeLoc;
  int strengthLoc;
  int modeLoc;
  int animPhaseLoc;
  int animRangeLoc;
  int preFoldLoc;
  float phase; // Animation accumulator
} ChladniWarpEffect;

// Returns true on success, false if shader fails to load
bool ChladniWarpEffectInit(ChladniWarpEffect *e);

// Accumulates phase, sets all uniforms
void ChladniWarpEffectSetup(ChladniWarpEffect *e, const ChladniWarpConfig *cfg,
                            float deltaTime);

// Unloads shader
void ChladniWarpEffectUninit(ChladniWarpEffect *e);

// Returns default config
ChladniWarpConfig ChladniWarpConfigDefault(void);

// Registers modulatable params with the modulation engine
void ChladniWarpRegisterParams(ChladniWarpConfig *cfg);

#endif // CHLADNI_WARP_H
