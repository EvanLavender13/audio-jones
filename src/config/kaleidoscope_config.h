#ifndef KALEIDOSCOPE_CONFIG_H
#define KALEIDOSCOPE_CONFIG_H

struct KaleidoscopeConfig {
    bool enabled = false;

    // Technique intensities (0.0 = disabled, 1.0 = full)
    float polarIntensity = 0.0f;        // Standard polar mirroring
    float kifsIntensity = 0.0f;         // Kaleidoscopic IFS fractal folding
    float drosteIntensity = 0.0f;       // Log-polar spiral (Escher-like)
    float iterMirrorIntensity = 0.0f;   // Iterative rotation + mirror
    float hexFoldIntensity = 0.0f;      // Hexagonal lattice symmetry
    float powerMapIntensity = 0.0f;     // Conformal z^n transform

    // Shared parameters
    int segments = 1;             // Mirror segments (1 = disabled, 4/6/8/12 common)
    float rotationSpeed = 0.002f; // Rotation rate (radians/tick)
    float twistAngle = 0.0f;      // Radial twist (radians, 0 = disabled)
    float focalAmplitude = 0.0f;  // Lissajous center offset (UV units, 0 = disabled)
    float focalFreqX = 1.0f;      // Lissajous X frequency
    float focalFreqY = 1.5f;      // Lissajous Y frequency
    float warpStrength = 0.0f;    // fBM warp intensity (0 = disabled)
    float warpSpeed = 0.1f;       // fBM animation speed
    float noiseScale = 2.0f;      // fBM spatial scale

    // KIFS-specific params
    int kifsIterations = 4;       // Folding iterations (1-8)
    float kifsScale = 2.0f;       // Per-iteration scale factor
    float kifsOffsetX = 1.0f;     // X translation after fold
    float kifsOffsetY = 1.0f;     // Y translation after fold

    // Droste-specific params
    float drosteScale = 4.0f;     // Zoom ratio per revolution (2.0-256.0)

    // Hex Fold-specific params
    float hexScale = 8.0f;        // Hex cell density (1.0-20.0)

    // Power Map-specific params
    float powerMapN = 2.0f;       // Power exponent (0.5-8.0)
};

#endif // KALEIDOSCOPE_CONFIG_H
