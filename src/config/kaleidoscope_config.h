#ifndef KALEIDOSCOPE_CONFIG_H
#define KALEIDOSCOPE_CONFIG_H

typedef enum {
    KALEIDO_POLAR = 0,  // Standard polar mirroring
    KALEIDO_KIFS        // Kaleidoscopic IFS fractal folding
} KaleidoscopeMode;

struct KaleidoscopeConfig {
    KaleidoscopeMode mode = KALEIDO_POLAR;
    int segments = 1;             // Mirror segments (1 = disabled, 4/6/8/12 common)
    float rotationSpeed = 0.002f; // Rotation rate (radians/tick)
    float twistAmount = 0.0f;     // Radial twist (radians, 0 = disabled)
    float focalAmplitude = 0.0f;  // Lissajous center offset (UV units, 0 = disabled)
    float focalFreqX = 1.0f;      // Lissajous X frequency
    float focalFreqY = 1.5f;      // Lissajous Y frequency
    float warpStrength = 0.0f;    // fBM warp intensity (0 = disabled)
    float warpSpeed = 0.1f;       // fBM animation speed
    float noiseScale = 2.0f;      // fBM spatial scale

    // KIFS mode params
    int kifsIterations = 4;       // Folding iterations (1-8)
    float kifsScale = 2.0f;       // Per-iteration scale factor
    float kifsOffsetX = 1.0f;     // X translation after fold
    float kifsOffsetY = 1.0f;     // Y translation after fold
};

#endif // KALEIDOSCOPE_CONFIG_H
