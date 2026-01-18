#ifndef TEXTURE_WARP_CONFIG_H
#define TEXTURE_WARP_CONFIG_H

enum class TextureWarpChannelMode {
    RG = 0,             // Red-Green channels
    RB = 1,             // Red-Blue channels
    GB = 2,             // Green-Blue channels
    Luminance = 3,      // Grayscale displacement
    LuminanceSplit = 4, // Opposite X/Y from luminance
    Chrominance = 5,    // Color difference channels
    Polar = 6           // Hue->angle, saturation->magnitude
};

struct TextureWarpConfig {
    bool enabled = false;
    float strength = 0.05f;   // Warp magnitude per iteration (0.0 to 0.3)
    int iterations = 3;       // Cascade depth (1 to 8)
    TextureWarpChannelMode channelMode = TextureWarpChannelMode::RG;
    float ridgeAngle = 0.0f;  // Ridge direction (radians)
    float anisotropy = 0.0f;  // 0=isotropic, 1=fully directional
    float noiseAmount = 0.0f; // Procedural noise blend (0.0 to 1.0)
    float noiseScale = 5.0f;  // Noise frequency
};

#endif // TEXTURE_WARP_CONFIG_H
