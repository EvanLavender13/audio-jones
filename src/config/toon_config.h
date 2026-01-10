#ifndef TOON_CONFIG_H
#define TOON_CONFIG_H

// Toon: Cartoon-style post-processing with luminance posterization and Sobel edge outlines
// Noise-varied edge thickness creates organic brush-stroke appearance
struct ToonConfig {
    bool enabled = false;
    int levels = 4;                    // Luminance quantization levels (2-16)
    float edgeThreshold = 0.2f;        // Edge detection sensitivity (0.0-1.0)
    float edgeSoftness = 0.05f;        // Edge antialiasing width (0.0-0.2)
    float thicknessVariation = 0.0f;   // Noise-based stroke variation (0.0-1.0)
    float noiseScale = 5.0f;           // Brush stroke noise frequency (1.0-20.0)
};

#endif // TOON_CONFIG_H
