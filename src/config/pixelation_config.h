#ifndef PIXELATION_CONFIG_H
#define PIXELATION_CONFIG_H

// Pixelation: UV quantization with optional Bayer dithering and color
// posterization Reduces image to mosaic cells for retro 8-bit aesthetic
struct PixelationConfig {
  bool enabled = false;
  float cellCount = 64.0f; // Cells across width (4-256). Lower = blockier.
  int posterizeLevels = 0; // Color levels per channel (0-16). 0 = disabled.
  float ditherScale =
      1.0f; // Dither pattern size (1-8). Only applies with posterize.
};

#endif // PIXELATION_CONFIG_H
