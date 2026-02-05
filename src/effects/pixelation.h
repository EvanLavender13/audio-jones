// Pixelation effect module
// UV quantization with optional Bayer dithering and color posterization

#ifndef PIXELATION_H
#define PIXELATION_H

#include "raylib.h"
#include <stdbool.h>

// Reduces image to mosaic cells for retro 8-bit aesthetic
struct PixelationConfig {
  bool enabled = false;
  float cellCount = 64.0f; // Cells across width (4-256). Lower = blockier.
  int posterizeLevels = 0; // Color levels per channel (0-16). 0 = disabled.
  float ditherScale =
      1.0f; // Dither pattern size (1-8). Only applies with posterize.
};

typedef struct PixelationEffect {
  Shader shader;
  int resolutionLoc;
  int cellCountLoc;
  int ditherScaleLoc;
  int posterizeLevelsLoc;
} PixelationEffect;

// Returns true on success, false if shader fails to load
bool PixelationEffectInit(PixelationEffect *e);

// Sets all uniforms
void PixelationEffectSetup(PixelationEffect *e, const PixelationConfig *cfg);

// Unloads shader
void PixelationEffectUninit(PixelationEffect *e);

// Returns default config
PixelationConfig PixelationConfigDefault(void);

// Registers modulatable params with the modulation engine
void PixelationRegisterParams(PixelationConfig *cfg);

#endif // PIXELATION_H
