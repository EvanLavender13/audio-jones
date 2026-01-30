#ifndef PALETTE_QUANTIZATION_CONFIG_H
#define PALETTE_QUANTIZATION_CONFIG_H

// Palette Quantization: Reduces image colors to a limited palette with ordered
// Bayer dithering Creates retro/pixel-art aesthetics from 8-color to 4096-color
// palettes without UV quantization
struct PaletteQuantizationConfig {
  bool enabled = false;
  float colorLevels = 4.0f; // Quantization levels per channel (2.0-16.0). 2=8
                            // colors, 4=64, 8=512.
  float ditherStrength =
      0.5f; // Dithering intensity (0.0-1.0). 0=hard bands, 1=full dither.
  int bayerSize = 8; // Dither matrix size (4 or 8). 4=coarser pattern, 8=finer.
};

#endif // PALETTE_QUANTIZATION_CONFIG_H
