// Matrix rain effect module
// Falling procedural rune columns with variable-speed trails

#ifndef MATRIX_RAIN_H
#define MATRIX_RAIN_H

#include "raylib.h"
#include <stdbool.h>

struct MatrixRainConfig {
  bool enabled = false;
  float cellSize = 12.0f;        // Cell size in pixels (4-32)
  float rainSpeed = 1.0f;        // Animation speed multiplier (0.1-5.0)
  float trailLength = 15.0f;     // Characters per rain strip (5-40)
  int fallerCount = 5;           // Rain drops per column (1-20)
  float overlayIntensity = 0.8f; // Rain opacity (0.0-1.0)
  float refreshRate = 1.0f;      // Character change frequency (0.1-5.0)
  float leadBrightness = 1.5f;   // Extra brightness on leading char (0.5-3.0)
  bool sampleMode = false; // Glyphs colored by source texture, gaps go black
};

typedef struct MatrixRainEffect {
  Shader shader;
  int resolutionLoc;
  int cellSizeLoc;
  int trailLengthLoc;
  int fallerCountLoc;
  int overlayIntensityLoc;
  int refreshRateLoc;
  int leadBrightnessLoc;
  int timeLoc;
  int sampleModeLoc;
  float time; // Rain animation accumulator
} MatrixRainEffect;

// Returns true on success, false if shader fails to load
bool MatrixRainEffectInit(MatrixRainEffect *e);

// Accumulates time, sets all uniforms
void MatrixRainEffectSetup(MatrixRainEffect *e, const MatrixRainConfig *cfg,
                           float deltaTime);

// Unloads shader
void MatrixRainEffectUninit(MatrixRainEffect *e);

// Returns default config
MatrixRainConfig MatrixRainConfigDefault(void);

#endif // MATRIX_RAIN_H
