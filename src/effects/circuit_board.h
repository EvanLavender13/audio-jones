#ifndef CIRCUIT_BOARD_H
#define CIRCUIT_BOARD_H

#include "raylib.h"
#include <stdbool.h>

// Circuit Board recursive fold pattern generator
// Stacks iterative coordinate folds with scale decay to create PCB trace-like
// patterns. Supports scrolling animation and chromatic RGB channel separation.
struct CircuitBoardConfig {
  bool enabled = false;
  float patternX = 7.0f;    // X component of pattern constant (1.0-10.0)
  float patternY = 5.0f;    // Y component of pattern constant (1.0-10.0)
  int iterations = 6;       // Recursion depth (3-12)
  float scale = 1.4f;       // Initial folding frequency (0.5-3.0)
  float offset = 0.16f;     // Initial offset between folds (0.05-0.5)
  float scaleDecay = 1.05f; // Scale reduction per iteration (1.01-1.2)
  float strength = 0.5f;    // Warp intensity (0.0-1.0)
  float scrollSpeed = 0.0f; // Animation scroll rate (0.0-2.0)
  float scrollAngle = 0.0f; // Scroll direction angle (radians)
  bool chromatic = false;   // Per-channel iteration for RGB separation
};

typedef struct CircuitBoardEffect {
  Shader shader;
  int patternConstLoc;
  int iterationsLoc;
  int scaleLoc;
  int offsetLoc;
  int scaleDecayLoc;
  int strengthLoc;
  int scrollOffsetLoc;
  int rotationAngleLoc;
  int chromaticLoc;
  float scrollOffset; // Accumulated scroll
} CircuitBoardEffect;

// Returns true on success, false if shader fails to load
bool CircuitBoardEffectInit(CircuitBoardEffect *e);

// Accumulates scrollOffset, sets all uniforms
void CircuitBoardEffectSetup(CircuitBoardEffect *e,
                             const CircuitBoardConfig *cfg, float deltaTime);

// Unloads shader
void CircuitBoardEffectUninit(CircuitBoardEffect *e);

// Returns default config
CircuitBoardConfig CircuitBoardConfigDefault(void);

#endif // CIRCUIT_BOARD_H
