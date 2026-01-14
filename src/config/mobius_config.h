#ifndef MOBIUS_CONFIG_H
#define MOBIUS_CONFIG_H

// Mobius Transform: two-point conformal warp with log-polar spiral animation
// Point1 defines transform center; point2 is the pole (singularity)
struct MobiusConfig {
    bool enabled = false;
    float point1X = 0.3f;            // Fixed point 1 X in UV space (0.0-1.0)
    float point1Y = 0.5f;            // Fixed point 1 Y (0.0-1.0)
    float point2X = 0.7f;            // Fixed point 2 X / pole in UV space (0.0-1.0)
    float point2Y = 0.5f;            // Fixed point 2 Y / pole (0.0-1.0)
    float spiralTightness = 0.0f;    // Spiral arm count in log-polar space (-2.0-2.0)
    float zoomFactor = 0.0f;         // Radial zoom multiplier (-2.0-2.0)
    float animRate = 1.0f;           // Animation rate (radians/second, 0.0-2.0)
    float pointAmplitude = 0.0f;     // Lissajous motion amplitude (0.0-0.3)
    float pointFreq1 = 1.0f;         // Point 1 oscillation frequency (0.1-5.0)
    float pointFreq2 = 1.3f;         // Point 2 oscillation frequency (0.1-5.0)
};

#endif // MOBIUS_CONFIG_H
