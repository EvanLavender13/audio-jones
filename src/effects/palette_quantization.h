// Palette quantization effect module
// Reduces colors to a limited palette with ordered Bayer dithering

#ifndef PALETTE_QUANTIZATION_H
#define PALETTE_QUANTIZATION_H

#include "raylib.h"
#include <stdbool.h>

struct PaletteQuantizationConfig {
  bool enabled = false;
  float colorLevels = 4.0f; // Quantization levels per channel (2.0-16.0). 2=8
                            // colors, 4=64, 8=512.
  float ditherStrength =
      0.5f; // Dithering intensity (0.0-1.0). 0=hard bands, 1=full dither.
  int bayerSize = 8; // Dither matrix size (4 or 8). 4=coarser pattern, 8=finer.
};

typedef struct PaletteQuantizationEffect {
  Shader shader;
  int colorLevelsLoc;
  int ditherStrengthLoc;
  int bayerSizeLoc;
} PaletteQuantizationEffect;

// Returns true on success, false if shader fails to load
bool PaletteQuantizationEffectInit(PaletteQuantizationEffect *e);

// Sets all uniforms
void PaletteQuantizationEffectSetup(PaletteQuantizationEffect *e,
                                    const PaletteQuantizationConfig *cfg);

// Unloads shader
void PaletteQuantizationEffectUninit(PaletteQuantizationEffect *e);

// Returns default config
PaletteQuantizationConfig PaletteQuantizationConfigDefault(void);

#endif // PALETTE_QUANTIZATION_H
