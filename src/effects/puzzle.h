#ifndef PUZZLE_H
#define PUZZLE_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

struct PuzzleConfig {
  bool enabled = false;

  // Geometry
  float pieceCount =
      12.0f;         // Pieces across screen height (4-40, integer slider)
  float seed = 0.0f; // Hash offset for tab/blank pattern (0-100)

  // Color
  int fillMode = 0; // 0 = Texture, 1 = Solid (cell-center color)

  // Lighting
  float edgeLight = 1.0f; // Edge lighting strength (0-1)
};

#define PUZZLE_CONFIG_FIELDS enabled, pieceCount, seed, fillMode, edgeLight

typedef struct PuzzleEffect {
  Shader shader;
  int resolutionLoc;
  int pieceCountLoc;
  int seedLoc;
  int fillModeLoc;
  int edgeLightLoc;
} PuzzleEffect;

bool PuzzleEffectInit(PuzzleEffect *e);
void PuzzleEffectSetup(const PuzzleEffect *e, const PuzzleConfig *cfg);
void PuzzleEffectUninit(const PuzzleEffect *e);
void PuzzleRegisterParams(PuzzleConfig *cfg);

PuzzleEffect *GetPuzzleEffect(PostEffect *pe);

#endif // PUZZLE_H
