#ifndef WATERCOLOR_CONFIG_H
#define WATERCOLOR_CONFIG_H

// Watercolor: Simulates watercolor painting through edge darkening (pigment pooling),
// procedural paper granulation, and soft color bleeding
struct WatercolorConfig {
    bool enabled = false;
    float edgeDarkening = 0.3f;        // Pigment pooling at edges (0.0-1.0)
    float granulationStrength = 0.3f;  // Paper texture intensity (0.0-1.0)
    float paperScale = 8.0f;           // Paper texture scale (1.0-20.0)
    float softness = 0.0f;             // Paint diffusion blur radius (0.0-5.0)
    float bleedStrength = 0.2f;        // Color bleeding at edges (0.0-1.0)
    float bleedRadius = 3.0f;          // Bleeding sample distance (1.0-10.0)
    int colorLevels = 0;               // Color quantization levels, 0 = disabled (0-16)
};

#endif // WATERCOLOR_CONFIG_H
