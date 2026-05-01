// Shard Crush effect module
// Iterative noise-driven angular shard distortion with chromatic aberration

#ifndef SHARD_CRUSH_H
#define SHARD_CRUSH_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

struct ShardCrushConfig {
  bool enabled = false;
  int iterations = 100;           // Loop count (20-100)
  float zoom = 0.4f;              // Base coordinate scale (0.1-2.0)
  float aberrationSpread = 0.01f; // Per-iteration UV offset (0.001-0.05)
  float noiseScale = 64.0f;       // Noise tiling scale (16.0-256.0)
  float rotationLevels = 8.0f;    // Angle quantization steps (2.0-16.0)
  float softness = 0.0f;          // Hard binary to smooth gradient (0.0-1.0)
  float speed = 1.0f;             // Animation rate (0.1-5.0)
  float mix = 1.0f;               // Blend with original (0.0-1.0)
};

#define SHARD_CRUSH_CONFIG_FIELDS                                              \
  enabled, iterations, zoom, aberrationSpread, noiseScale, rotationLevels,     \
      softness, speed, mix

typedef struct ShardCrushEffect {
  Shader shader;
  int resolutionLoc;
  int timeLoc;
  int iterationsLoc;
  int zoomLoc;
  int aberrationSpreadLoc;
  int noiseScaleLoc;
  int rotationLevelsLoc;
  int softnessLoc;
  int mixLoc;
  float time;
} ShardCrushEffect;

// Returns true on success, false if shader fails to load
bool ShardCrushEffectInit(ShardCrushEffect *e);

// Accumulates time and sets all uniforms
void ShardCrushEffectSetup(ShardCrushEffect *e, const ShardCrushConfig *cfg,
                           float deltaTime);

// Unloads shader
void ShardCrushEffectUninit(const ShardCrushEffect *e);

// Registers modulatable params with the modulation engine
void ShardCrushRegisterParams(ShardCrushConfig *cfg);

ShardCrushEffect *GetShardCrushEffect(PostEffect *pe);

#endif // SHARD_CRUSH_H
