#ifndef KALEIDOSCOPE_CONFIG_H
#define KALEIDOSCOPE_CONFIG_H

// Kaleidoscope (Polar): Wedge-based radial mirroring with optional smooth edges
struct KaleidoscopeConfig {
    bool enabled = false;
    int segments = 6;             // Mirror segments / wedge count (1-12)
    float rotationSpeed = 0.002f; // Rotation rate (radians/frame)
    float twistAngle = 0.0f;      // Radial twist offset (radians)
    float smoothing = 0.0f;       // Blend width at wedge seams (0.0-0.5, 0 = hard edge)
    float focalAmplitude = 0.0f;  // Lissajous center offset (UV units, 0 = disabled)
    float focalFreqX = 1.0f;      // Lissajous X frequency
    float focalFreqY = 1.5f;      // Lissajous Y frequency
    float warpStrength = 0.0f;    // fBM warp intensity (0 = disabled)
    float warpSpeed = 0.1f;       // fBM animation speed
    float noiseScale = 2.0f;      // fBM spatial scale

    // Deprecated: kept for backward compatibility until Phase 3/4/6 migrate usage
    float polarIntensity = 0.0f;
    float kifsIntensity = 0.0f;
    float iterMirrorIntensity = 0.0f;
    float hexFoldIntensity = 0.0f;
    int kifsIterations = 4;
    float kifsScale = 2.0f;
    float kifsOffsetX = 1.0f;
    float kifsOffsetY = 1.0f;
    float hexScale = 8.0f;
};

#endif // KALEIDOSCOPE_CONFIG_H
