// Flip Book effect module
// Holds frames at a configurable FPS with per-frame jitter wobble

#ifndef FLIP_BOOK_H
#define FLIP_BOOK_H

#include "raylib.h"
#include <stdbool.h>

struct PostEffect;

struct FlipBookConfig {
  bool enabled = false;
  float fps = 12.0f;   // Target frame rate (2.0-60.0)
  float jitter = 2.0f; // Wobble intensity in pixels per held frame (0.0-8.0)
};

#define FLIP_BOOK_CONFIG_FIELDS enabled, fps, jitter

typedef struct FlipBookEffect {
  Shader shader;
  RenderTexture2D heldFrame;
  float frameTimer;      // Accumulated time since last frame capture
  int frameIndex;        // Integer seed, incremented on capture
  int lastRenderedIndex; // Tracks whether heldFrame needs update

  // Uniform locations
  int resolutionLoc;
  int jitterSeedLoc;
  int jitterAmountLoc;
} FlipBookEffect;

// Loads shader, caches uniform locations, allocates held frame texture
bool FlipBookEffectInit(FlipBookEffect *e, int width, int height);

// Accumulates frame timer, captures frames at target FPS, binds uniforms
void FlipBookEffectSetup(FlipBookEffect *e, const FlipBookConfig *cfg,
                         float deltaTime);

// Renders held frame with jitter offset to pipeline destination
void FlipBookEffectRender(FlipBookEffect *e, const PostEffect *pe);

// Reallocates held frame texture at new dimensions
void FlipBookEffectResize(FlipBookEffect *e, int width, int height);

// Unloads shader and held frame texture
void FlipBookEffectUninit(const FlipBookEffect *e);

// Registers modulatable params with the modulation engine
void FlipBookRegisterParams(FlipBookConfig *cfg);

FlipBookEffect *GetFlipBookEffect(PostEffect *pe);

#endif // FLIP_BOOK_H
