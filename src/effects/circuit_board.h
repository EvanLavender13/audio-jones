#ifndef CIRCUIT_BOARD_H
#define CIRCUIT_BOARD_H

#include "raylib.h"
#include <stdbool.h>

// Circuit Board Voronoi grid warp
// Tiles space into SDF square Voronoi cells with breathing animation, optional
// dual layers, and contour-band displacement for PCB trace aesthetics.
struct CircuitBoardConfig {
  bool enabled = false;
  float tileScale = 8.0f;    // Grid density (2.0-16.0)
  float strength = 0.3f;     // Warp displacement intensity (0.0-1.0)
  float baseSize = 0.7f;     // Static cell radius before animation (0.3-0.9)
  float breathe = 0.2f;      // Cell size oscillation amplitude (0.0-0.4)
  float breatheSpeed = 1.0f; // Cell size oscillation rate (0.1-4.0)
  bool dualLayer = false;    // Enable second overlapping grid evaluation
  float layerOffset = 40.0f; // Spatial offset between grids (5.0-80.0)
  float contourFreq = 0.0f;  // Periodic contour band frequency (0.0-80.0)
};

#define CIRCUIT_BOARD_CONFIG_FIELDS                                            \
  enabled, tileScale, strength, baseSize, breathe, breatheSpeed, dualLayer,    \
      layerOffset, contourFreq

typedef struct CircuitBoardEffect {
  Shader shader;
  int tileScaleLoc;
  int strengthLoc;
  int baseSizeLoc;
  int breatheLoc;
  int timeLoc;
  int dualLayerLoc;
  int layerOffsetLoc;
  int contourFreqLoc;
  float time; // Accumulated animation time
} CircuitBoardEffect;

// Returns true on success, false if shader fails to load
bool CircuitBoardEffectInit(CircuitBoardEffect *e);

// Accumulates animation time, sets all uniforms
void CircuitBoardEffectSetup(CircuitBoardEffect *e,
                             const CircuitBoardConfig *cfg, float deltaTime);

// Unloads shader
void CircuitBoardEffectUninit(CircuitBoardEffect *e);

// Returns default config
CircuitBoardConfig CircuitBoardConfigDefault(void);

// Registers modulatable params with the modulation engine
void CircuitBoardRegisterParams(CircuitBoardConfig *cfg);

#endif // CIRCUIT_BOARD_H
