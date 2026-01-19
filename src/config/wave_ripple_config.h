#ifndef WAVE_RIPPLE_CONFIG_H
#define WAVE_RIPPLE_CONFIG_H

// Wave Ripple: pseudo-3D radial wave displacement
// Summed sine waves create height field; gradient displaces UVs for parallax
struct WaveRippleConfig {
    bool enabled = false;
    int octaves = 2;              // Wave octaves (1-4)
    float strength = 0.02f;       // UV displacement strength (0.0-0.5)
    float animRate = 1.0f;        // Animation rate (radians/second, 0.0-5.0)
    float frequency = 8.0f;       // Base wave frequency (1.0-20.0)
    float steepness = 0.0f;       // Gerstner asymmetry: 0=sine, 1=sharp crests (0.0-1.0)
    float centerHole = 0.0f;      // Radius of calm center (0.0-0.5 UV space)
    float originX = 0.5f;         // Wave origin X in UV space (0.0-1.0)
    float originY = 0.5f;         // Wave origin Y in UV space (0.0-1.0)
    float originAmplitude = 0.0f; // Lissajous motion amplitude (0.0-0.3)
    float originFreqX = 1.0f;     // Origin X oscillation frequency (0.1-5.0)
    float originFreqY = 1.0f;     // Origin Y oscillation frequency (0.1-5.0)
    bool shadeEnabled = false;    // Height-based brightness modulation
    float shadeIntensity = 0.2f;  // Shade strength (0.0-0.5)
};

#endif // WAVE_RIPPLE_CONFIG_H
