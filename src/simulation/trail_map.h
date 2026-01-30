#ifndef TRAIL_MAP_H
#define TRAIL_MAP_H

#include "raylib.h"
#include <stdbool.h>

typedef struct TrailMap {
  RenderTexture2D primary; // Main trail texture (agents write here)
  RenderTexture2D temp;    // Ping-pong buffer for diffusion
  unsigned int program;    // Trail compute shader program

  // Uniform locations
  int resolutionLoc;
  int diffusionScaleLoc;
  int decayFactorLoc;
  int applyDecayLoc;
  int directionLoc;

  int width;
  int height;
} TrailMap;

// Initialize trail map with given dimensions. Returns NULL on failure.
TrailMap *TrailMapInit(int width, int height);

// Release all trail map resources.
void TrailMapUninit(TrailMap *tm);

// Recreate textures at new dimensions.
void TrailMapResize(TrailMap *tm, int width, int height);

// Clear trail textures to black.
void TrailMapClear(TrailMap *tm);

// Run diffusion and decay compute pass.
void TrailMapProcess(TrailMap *tm, float deltaTime, float decayHalfLife,
                     int diffusionScale);

// Begin drawing to the primary trail texture. Returns false if unavailable.
bool TrailMapBeginDraw(TrailMap *tm);

// End drawing to the primary trail texture.
void TrailMapEndDraw(TrailMap *tm);

// Get the primary trail texture for sampling.
Texture2D TrailMapGetTexture(const TrailMap *tm);

#endif // TRAIL_MAP_H
