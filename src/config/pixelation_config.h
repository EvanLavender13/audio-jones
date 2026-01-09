#ifndef PIXELATION_CONFIG_H
#define PIXELATION_CONFIG_H

// Pixelation: UV quantization with optional Bayer dithering and color posterization
// Reduces image to mosaic cells for retro 8-bit aesthetic
struct PixelationConfig {
    bool enabled = false;
    float cellCount = 64.0f;     // Cells across width (4-256). Lower = blockier.
    float ditherScale = 0.0f;    // Dither pattern size (0-8). 0 = disabled.
    int posterizeLevels = 0;     // Color levels per channel (0-16). 0 = disabled.
};

#endif // PIXELATION_CONFIG_H
